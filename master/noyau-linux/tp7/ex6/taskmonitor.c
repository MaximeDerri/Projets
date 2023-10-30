// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/pid.h>
#include <linux/kernel_stat.h>
#include <linux/sched/task.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/shrinker.h>
#include <linux/mempool.h>
#include <linux/kref.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/fs.h>

MODULE_DESCRIPTION("monitor CPU stat of one process module");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

#define BUFF_SIZE 100 //pour les prints
#define COUNT_LIMIT 50 //on gardera ça ou moins en cas de pression mémoire
#define MEMPOOL_MIN 20 //init mempool

static pid_t process_id = 1;
module_param(process_id, int, 0660); //pid_t <-> int

struct task_sample {
	struct list_head list;
	struct kref ref;
	u64 utime;
	u64 stime;
};

struct task_monitor {
	struct pid *pid;
	struct list_head list;
	struct mutex mtx;
	u64 size;
};


//structure qui rerefence le thread ("process descriptor")
//contient toutes les informations dont le noyau a besoin a propos de la tache
static struct task_struct *thread;
static struct task_monitor task; //tache a surveiller
static struct shrinker sh;
struct kmem_cache *taskmonitor_cache;
static mempool_t *taskmonitor_mempool;
static struct dentry *dfs;


static void release_task_sample(struct kref *ref)
{
	struct task_sample *ts = container_of(ref, struct task_sample, ref);
	mutex_lock(&task.mtx); //pour manipuler la liste
	list_del(&ts->list);
	mempool_free(ts, taskmonitor_mempool);
	--(task.size);
	mutex_unlock(&task.mtx);
}

static unsigned long taskmonitor_count_objects(struct shrinker *shrink,
				struct shrink_control *shrink_ctl)
{
	//pr_warn("in count\n");
	return (task.size > COUNT_LIMIT) ? task.size - COUNT_LIMIT
		: SHRINK_EMPTY;
}

static unsigned long taskmonitor_scan_objects(struct shrinker *shrink,
				struct shrink_control *shrink_ctl)
{
	struct task_sample *ts, *ts2;
	unsigned long to_del = shrink_ctl->nr_to_scan;

	pr_warn("size before remove, size = %llu\n", task.size);
	pr_warn("to remove - scan: %lu\n", to_del);
	list_for_each_entry_safe_reverse(ts, ts2, &task.list, list) {
		kref_put(&ts->ref, release_task_sample);

		if (--to_del <= 0)
			break;
	}

	pr_warn("after remove, size = %llu\n", task.size);
	return shrink_ctl->nr_to_scan - to_del;
}

static s8 get_sample(struct task_monitor *tm, struct task_sample *sample)
{
	struct task_struct *task_of_pid = get_pid_task(tm->pid, PIDTYPE_PID);

	if (task_of_pid == NULL)
		return 0;

	if (pid_alive(task_of_pid) == 0) {
		put_task_struct(task_of_pid);
		return 0;
	}

	sample->utime = task_of_pid->utime;
	sample->stime = task_of_pid->stime;
	put_task_struct(task_of_pid);
	return 1;
}

static long save_sample(void)
{
	struct task_sample *ts;
	s8 ret;

	mutex_lock(&task.mtx);
	ts = mempool_alloc(taskmonitor_mempool, GFP_KERNEL);

	if (ts == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	ts->stime = ts->utime = 0;
	ret = get_sample(&task, ts);
	kref_init(&ts->ref);
	list_add_tail(&ts->list, &task.list);
	++(task.size);
err:
	mutex_unlock(&task.mtx);
	return ret;
}

static int monitor_pid(pid_t id)
{
	task.pid = find_get_pid(id);
	if (task.pid == NULL) {
		pr_err("struct pid not found\n");
		return -ESRCH; //le module ne sera donc pas disponible
	}

	return 0;
}

static int monitor_fn(void *unused)
{
	while (!kthread_should_stop()) {
		save_sample();

		//sleep
		set_current_state(TASK_UNINTERRUPTIBLE);
		//HZ = nbr de ticks par sec / nbr de ++ pour jiffies par sec
		schedule_timeout(HZ);
	}

	return 0;
}

static ssize_t taskmonitor_store(struct kobject *kobj,
				struct kobj_attribute *attr, const char *buff,
				size_t count)
{
	//recup les char et verifier avec strcmp
	char msg[7]; //max = start\n\0 = 7 char
	int rec;

	if (count > 7) { //verif la taille
		pr_warn("data is too large - expected 7 char max\n");
		return count;
	}

	rec = sscanf(buff, "%s", msg);
	if (rec <= 0)
		return -EINVAL;

	if ((strcmp(msg, "start\n") == 0) || (strcmp(msg, "start") == 0)) {
		if (!thread) { //si le thread est en cours, on en relance pas
			thread = kthread_run(monitor_fn, NULL, "monitor_fn");
			if (IS_ERR(thread)) {
				pr_warn("kthread_run error - start msg: %ld\n",
					PTR_ERR(thread));
			}
			pr_warn("thread was created\n");
		} else
			pr_warn("thread is already running\n");
	} else if ((strcmp(msg, "stop\n") == 0) || (strcmp(msg, "stop") == 0)) {
		if (thread) { //le thread existe
			kthread_stop(thread);
			thread = NULL;
			pr_warn("thread was killed\n");
		} else
			pr_warn("thread is already dead\n");
	} else
		pr_warn("taskmonitor_store received a bad command\n");

	return count;
}
static struct kobj_attribute kobj_attr = __ATTR_WO(taskmonitor);

//dans exit, pour rendre les objets
static void free_taskmonitor_cache(void)
{
	struct task_sample *ts, *ts2;

	list_for_each_entry_safe(ts, ts2, &task.list, list) {
		list_del(&ts->list);
		mempool_free(ts, taskmonitor_mempool);
	}
}

static void *taskmonitor_start(struct seq_file *sf, loff_t *pos)
{
	loff_t n = *pos;
	struct task_sample *ts;

	seq_printf(sf, "---  size = %lld  ---\n", task.size);
	mutex_lock(&task.mtx);
	list_for_each_entry(ts, &task.list, list) {
		if (n-- > 0)
			continue;
		if (kref_get_unless_zero(&ts->ref) == 0)
			continue;
		goto out;
	}

	ts = NULL;
out:
	return ts;
}

static void taskmonitor_stop(struct seq_file *sf, void *v)
{
	if (v)
		kref_put(&((struct task_sample *)v)->ref, release_task_sample);
	mutex_unlock(&task.mtx);
}

static void *taskmonitor_next(struct seq_file *sf, void *v, loff_t *pos)
{
	struct task_sample *it = v, *next = NULL, *prev = v;

	++(*pos);
	list_for_each_entry_continue(it, &task.list, list) {
		if (kref_get_unless_zero(&it->ref) == 0)
			continue;
		next = it;
		break;
	}
	kref_put(&prev->ref, release_task_sample);

	return next;
}

static int taskmonitor_show(struct seq_file *sf, void *v)
{
	struct task_sample *ts = v;

	seq_printf(sf, "pid %d usr %llu sys %llu\n", process_id,
		ts->utime, ts->stime);
	return 0;
}

static struct seq_operations sops = {
	.start = taskmonitor_start,
	.stop = taskmonitor_stop,
	.show = taskmonitor_show,
	.next = taskmonitor_next
};

static int taskmonitor_open(struct inode *ino, struct file *f)
{
	pr_warn("open\n");
	return seq_open(f, &sops);
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.llseek = seq_lseek,
	.open = taskmonitor_open,
	.read = seq_read,
	.release = seq_release
};

static int __init module_init_taskmonitor(void)
{
	int ret = 0;

	ret = monitor_pid(process_id); //recuperer la struct pid
	if (ret)
		goto ret;

	//debugfs
	dfs = debugfs_create_file("taskmonitor", 0660, NULL, NULL, &fops);
	if (dfs == NULL) {
		ret = -EINVAL;
		goto err_1;
	}

	//sysfs
	ret = sysfs_create_file(kernel_kobj, &kobj_attr.attr);
	if (ret)
		goto err_2;

	//thread noyau
	thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	if (IS_ERR(thread)) {
		ret = PTR_ERR(thread);
		goto err_3;
	}

	ret = register_shrinker(&sh);
	if (ret != 0)
		goto err_3;

	taskmonitor_cache = KMEM_CACHE(task_sample, 0);
	if (taskmonitor_cache == NULL) {
		ret = -ENOMEM;
		goto err_3;
	}

	taskmonitor_mempool = mempool_create(MEMPOOL_MIN,
					mempool_alloc_slab, mempool_free_slab,
					taskmonitor_cache);
	if (taskmonitor_mempool == NULL) {
		ret = -ENOMEM;
		goto err_4;
	}

	sh.count_objects = taskmonitor_count_objects;
	sh.scan_objects = taskmonitor_scan_objects;
	sh.seeks = DEFAULT_SEEKS;
	mutex_init(&task.mtx);
	INIT_LIST_HEAD(&task.list);
	task.size = 0;


	pr_warn("init\n");
	return 0;

err_4:
	kmem_cache_destroy(taskmonitor_cache);
err_3:
	sysfs_remove_file(kernel_kobj, &kobj_attr.attr);
err_2:
	debugfs_remove(dfs);
err_1:
	put_pid(task.pid);
ret:
	return ret;
}
module_init(module_init_taskmonitor);

static void __exit module_exit_taskmonitor(void)
{
	if (thread)
		kthread_stop(thread);

	if (task.pid)
		put_pid(task.pid);

	sysfs_remove_file(kernel_kobj, &kobj_attr.attr);
	debugfs_remove(dfs);

	unregister_shrinker(&sh);

	//liberer la memoire
	free_taskmonitor_cache();
	kmem_cache_destroy(taskmonitor_cache);
	mempool_destroy(taskmonitor_mempool);

	pr_warn("exit\n");
}
module_exit(module_exit_taskmonitor);
