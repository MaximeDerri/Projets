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

MODULE_DESCRIPTION("monitor CPU stat of one process module");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

#define BUFF_SIZE 100

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
	struct task_sample *ts = kzalloc(sizeof(struct task_sample), GFP_KERNEL);
	s8 ret;

	if (ts == NULL)
		return -ENOMEM;

	ts->stime = ts->utime = 0;
	mutex_lock(&task.mtx);
	ret = get_sample(&task, ts);
	list_add_tail(&ts->list, &task.list);
	++(task.size);
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
	struct task_sample *ts, *tmp;

	if (thread)
		kthread_stop(thread);

	if (task.pid)
		put_pid(task.pid);

	sysfs_remove_file(kernel_kobj, &kobj_attr.attr);

	//liberer la memoire
	list_for_each_entry_safe(ts, tmp, &task.list, list) {
		kfree(ts);
		list_del(&ts->list);
	}

	pr_warn("exit\n");
}
module_exit(module_exit_taskmonitor);
