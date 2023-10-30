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
#include <linux/uaccess.h>

MODULE_DESCRIPTION("monitor CPU stat of one process module");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

#define BUFF_SIZE 100 //pour les prints
#define BUFF_WRITE_SIZE 30
#define COUNT_LIMIT 30 //on gardera ça ou moins en cas de pression mémoire
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
	struct list_head others; // pref / next task_monitor
	struct mutex mtx;
	pid_t _pid;
	u64 size;
};
u64 size; //commun a chaque struct car resultats faits en même temps


//structure qui rerefence le thread ("process descriptor")
//contient toutes les informations dont le noyau a besoin a propos de la tache
static struct task_struct *thread;
static struct shrinker sh;
struct kmem_cache *taskmonitor_cache;
static mempool_t *taskmonitor_mempool;
static struct dentry *dfs;
static struct task_monitor tasks;
static struct task_monitor *task_monitor_to_show; //eviter de rechercher tout le temps dans la liste


/* ******************** */
//copie de nolibc/nolibc.h
//utilise pour ajouter/retirer/choisir pid a afficher

static __attribute__((unused))
long atol(const char *s)
{
	unsigned long ret = 0;
	unsigned long d;
	int neg = 0;

	if (*s == '-') {
		neg = 1;
		s++;
	}

	while (1) {
		d = (*s++) - '0';
		if (d > 9)
			break;
		ret *= 10;
		ret += d;
	}

	return neg ? -ret : ret;
}

static __attribute__((unused))
int atoi(const char *s)
{
	return atol(s);
}

/* ******************** */

static void set_new_process_to_show(pid_t process_to_show)
{
	struct task_monitor *it, *prev;

	prev = task_monitor_to_show;
	pr_warn("changing monitoring\n");
	list_for_each_entry(it, &tasks.others, others) {
		if (it->_pid == process_to_show && it->_pid >= 0) {
			mutex_lock(&prev->mtx); //si en cours d'affichage
			task_monitor_to_show = it;
			mutex_unlock(&prev->mtx);
			break;
		}
	}
}

static void release_task_sample(struct kref *ref)
{
	struct task_sample *ts = container_of(ref, struct task_sample, ref);

	list_del(&ts->list);
	mempool_free(ts, taskmonitor_mempool);
}

static unsigned long taskmonitor_count_objects(struct shrinker *shrink,
				struct shrink_control *shrink_ctl)
{
	//on adapte le code: si jamais la taille depasse la macro,
	//on retourne une valeur positive et scan_objects fera le tour des
	//processus monitorees pour supprimer le surplus de task_sample
	struct task_monitor *it;
	unsigned long val = 0;

	list_for_each_entry(it, &tasks.others, others) {
		val = (it->size > COUNT_LIMIT) ? it->size - COUNT_LIMIT
						: SHRINK_EMPTY;
		if (val > 0)
			return val;
	}
	return 0;
}

static unsigned long taskmonitor_scan_objects(struct shrinker *shrink,
				struct shrink_control *shrink_ctl)
{
	struct task_sample *ts, *ts2;
	struct task_monitor *it;
	unsigned long to_del = 0;//shrink_ctl->nr_to_scan;;
	unsigned long ret = 0;

	pr_warn("in scan_objects\n");

	list_for_each_entry(it, &tasks.others, others) {
		if (it->size > COUNT_LIMIT)
			to_del = it->size - COUNT_LIMIT; //on evalue la quantite
		else
			continue; //pas besoin de reduire

		mutex_lock(&it->mtx);
		//parcours des task_sample
		list_for_each_entry_safe_reverse(ts, ts2, &it->list, list) {
			kref_put(&ts->ref, release_task_sample);
			--(it->size);
			if (--to_del > 0)
				break;
		}
		ret = (to_del > ret) ? to_del : ret;
		mutex_unlock(&it->mtx);
	}
	return ret;

}

static int monitor_pid(pid_t id)
{
	struct pid *tmp;
	struct task_monitor *new_tm, *it;

	//pas inserer un pid deja present
	list_for_each_entry(it, &tasks.others, others) {
		if (it->_pid == id)
			return 0;
	}

	tmp = find_get_pid(id);
	if (tmp == NULL) {
		pr_err("struct pid not found: %d\n", id);
		return -ESRCH; //le module ne sera donc pas disponible
	}

	new_tm = kmalloc(sizeof(struct task_monitor), GFP_KERNEL);
	if (new_tm == NULL) {
		put_pid(tmp);
		return -ENOMEM;
	}

	new_tm->pid = tmp;
	new_tm->_pid = id;
	new_tm->size = 0;
	mutex_init(&new_tm->mtx);
	INIT_LIST_HEAD(&new_tm->others);
	INIT_LIST_HEAD(&new_tm->list);
	list_add(&new_tm->others, &tasks.others);

	if (task_monitor_to_show == NULL) //init
		task_monitor_to_show = new_tm;

	pr_warn("pid %d add\n", id);
	return 0;
}

static int remove_monitor_pid(pid_t id)
{
	struct task_monitor *tm;
	struct task_sample *ts, *ts2;

	list_for_each_entry(tm, &tasks.others, others) {
		if (tm->_pid < 0)
			continue;
		if (tm->_pid == id) { //trouve
			mutex_lock(&tm->mtx);
			list_del(&tm->others); //empeche l'acces

			//on vide
			list_for_each_entry_safe(ts, ts2, &tm->list, list) {
				list_del(&ts->list);
				mempool_free(ts, taskmonitor_mempool);
			}
			put_pid(tm->pid);
			if (task_monitor_to_show == tm)
				task_monitor_to_show = NULL;
			mutex_unlock(&tm->mtx);
			kfree(tm); //on libere | ne peut pas etre le pid bouchon
			pr_warn("pid %d removed\n", id);

			break;
		}
	}
	return 0;
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

static long save_sample(struct task_monitor *tm)
{
	struct task_sample *ts;
	s8 ret;

	mutex_lock(&tm->mtx); //PROBLEME
	ts = mempool_alloc(taskmonitor_mempool, GFP_KERNEL);

	if (ts == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	ts->stime = ts->utime = 0;
	ret = get_sample(tm, ts);
	kref_init(&ts->ref);
	INIT_LIST_HEAD(&ts->list);
	list_add_tail(&ts->list, &tm->list);
	++(tm->size);
err:
	mutex_unlock(&tm->mtx);
	return ret;
}

static int monitor_fn(void *unused)
{
	struct task_monitor *it;

	while (!kthread_should_stop()) {
		list_for_each_entry(it, &tasks.others, others) {
			if (it->_pid >= 0)
				save_sample(it);
		}

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




static void *taskmonitor_start(struct seq_file *sf, loff_t *pos)
{
	loff_t n = *pos;
	struct task_sample *ts;

	if (task_monitor_to_show == NULL) {
		pr_warn("task_monitor to show is NULL, you must chande the pid to print\n");
		return NULL;
	}

	mutex_lock(&task_monitor_to_show->mtx);
	list_for_each_entry(ts, &task_monitor_to_show->list, list) {
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

	if (task_monitor_to_show != NULL)
		mutex_unlock(&task_monitor_to_show->mtx);
}

static void *taskmonitor_next(struct seq_file *sf, void *v, loff_t *pos)
{
	struct task_sample *it = v, *next = NULL, *prev = v;

	++(*pos);
	list_for_each_entry_continue(it, &task_monitor_to_show->list, list) {
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

	seq_printf(sf, "pid %d usr %llu sys %llu\n", task_monitor_to_show->_pid,
		ts->utime, ts->stime);
	return 0;
}

static const struct seq_operations sops = {
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

static ssize_t taskmonitor_write(struct file *file, const char *buff, size_t sz, loff_t *ppos)
{
	//pid
	//-pid
	//print pid
	char tmp[BUFF_WRITE_SIZE];
	int ret;

	if (sz > 5) {
		if (strncmp(buff, "print ", 6) == 0)  {
			ret = copy_from_user(tmp, buff+6, BUFF_WRITE_SIZE);
			tmp[BUFF_WRITE_SIZE] = '\0'; //si ca depasse, pour eviter des buffer overflow
			set_new_process_to_show((pid_t)atoi(tmp));
			return sz;
		}
	}

	ret = copy_from_user(tmp, buff, BUFF_WRITE_SIZE);
	if (tmp[0] == '-')
		remove_monitor_pid((pid_t)atoi(tmp+1));
	else
		monitor_pid((pid_t)atoi(tmp));

	return sz;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.llseek = seq_lseek,
	.open = taskmonitor_open,
	.read = seq_read,
	.write = taskmonitor_write,
	.release = seq_release
};

/* *************************** */

//dans exit, pour rendre les objets
static void free_taskmonitor_cache(void)
{
	struct task_sample *ts, *ts2;
	struct task_monitor *tm, *tm2;

	list_for_each_entry_safe(tm, tm2, &tasks.others, others) {
		list_del(&tm->others);
		if (tm->_pid >= 0) {
			list_for_each_entry_safe(ts, ts2, &tm->list, list) {
				list_del(&ts->list);
				mempool_free(ts, taskmonitor_mempool);
			}
		kfree(tm);
		}
	}

}

static int __init module_init_taskmonitor(void)
{
	int ret = 0;

	INIT_LIST_HEAD(&tasks.list);
	INIT_LIST_HEAD(&tasks.others);
	tasks._pid = -1;

	task_monitor_to_show = NULL;
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


	pr_warn("init\n");
	return 0;

err_4:
	kmem_cache_destroy(taskmonitor_cache);
err_3:
	sysfs_remove_file(kernel_kobj, &kobj_attr.attr);
err_2:
	debugfs_remove(dfs);
err_1:
	put_pid(task_monitor_to_show->pid);
	kfree(task_monitor_to_show);
ret:
	return ret;
}
module_init(module_init_taskmonitor);

static void __exit module_exit_taskmonitor(void)
{
	struct task_monitor *tm;

	if (thread)
		kthread_stop(thread);

	list_for_each_entry(tm, &tasks.list, others) {
		if (tm->pid && tm->_pid >= 0)
			put_pid(tm->pid);
	}

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








