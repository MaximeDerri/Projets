// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/pid.h>
#include <linux/kernel_stat.h>
#include <linux/sched/task.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/shrinker.h>

MODULE_DESCRIPTION("monitor CPU stat of one process module");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

#define BUFF_SIZE 100
#define COUNT_LIMIT 50

static pid_t process_id = 1;
module_param(process_id, int, 0660); //pid_t <-> int

struct task_sample {
	struct list_head list;
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
	mutex_lock(&task.mtx);
	list_for_each_entry_safe_reverse(ts, ts2, &task.list, list) {
		list_del(&ts->list);
		kmem_cache_free(taskmonitor_cache, ts);
		--(task.size);

		if (--to_del <= 0)
			break;
	}

	pr_warn("after remove, size = %llu\n", task.size);
	mutex_unlock(&task.mtx);
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
	ts = kmem_cache_alloc(taskmonitor_cache, GFP_KERNEL);

	if (ts == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	//pr_warn("actual size = %lu | expected size = %lu\n", ksize(ts), sizeof(struct task_sample));
	ts->stime = ts->utime = 0;
	ret = get_sample(&task, ts);
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

static ssize_t taskmonitor_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buff)
{
	struct task_sample *cur;
	ssize_t cur_sz = 0;
	ssize_t tmp_sz = 0;
	char tmp[BUFF_SIZE];

	mutex_lock(&task.mtx);
	list_for_each_entry(cur, &task.list, list) {
		tmp_sz = snprintf(tmp, BUFF_SIZE, "pid %d usr %llu sys %llu\n",
				process_id, cur->utime, cur->stime);
		if (tmp_sz + cur_sz + 1 > PAGE_SIZE) //plus de place ?
			break;

		snprintf(buff + cur_sz, tmp_sz + 1, "%s", tmp); // +1 pour \0
		cur_sz += tmp_sz;
	}

	mutex_unlock(&task.mtx);
	return cur_sz;
}

//dans exit, pour rendre les objets
static void free_taskmonitor_cache(void)
{
	struct task_sample *ts, *ts2;

	list_for_each_entry_safe(ts, ts2, &task.list, list) {
		list_del(&ts->list);
		kmem_cache_free(taskmonitor_cache, ts);
	}
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
static struct kobj_attribute kobj_attr = __ATTR_RW(taskmonitor);

static int __init module_init_taskmonitor(void)
{
	int ret = 0;

	ret = monitor_pid(process_id); //recuperer la struct pid
	if (ret)
		goto ret;

	//sysfs
	ret = sysfs_create_file(kernel_kobj, &kobj_attr.attr);
	if (ret)
		goto err_1;

	//thread noyau
	thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	if (IS_ERR(thread)) {
		ret = PTR_ERR(thread);
		goto err_2;
	}

	ret = register_shrinker(&sh);
	if (ret != 0)
		goto err_2;

	taskmonitor_cache = KMEM_CACHE(task_sample, 0);
	if (taskmonitor_cache == NULL) {
		ret = -ENOMEM;
		goto err_2;
	}

	sh.count_objects = taskmonitor_count_objects;
	sh.scan_objects = taskmonitor_scan_objects;
	sh.seeks = DEFAULT_SEEKS;
	mutex_init(&task.mtx);
	INIT_LIST_HEAD(&task.list);
	task.size = 0;

	pr_warn("init\n");
	return 0;

err_2:
	sysfs_remove_file(kernel_kobj, &kobj_attr.attr);
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

	unregister_shrinker(&sh);

	//liberer la memoire
	free_taskmonitor_cache();
	kmem_cache_destroy(taskmonitor_cache);


	pr_warn("exit\n");
}
module_exit(module_exit_taskmonitor);
