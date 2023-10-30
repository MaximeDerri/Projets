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

MODULE_DESCRIPTION("monitor CPU stat of one process module");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

#define BUFF_SIZE 7

static pid_t process_id = 1;
module_param(process_id, int, 0660); //pid_t <-> int

struct task_monitor {
	struct pid *pid;
};

struct task_sample {
	u64 utime;
	u64 stime;
};


//structure qui rerefence le thread ("process descriptor")
//contient toutes les informations dont le noyau a besoin a propos de la tache
static struct task_struct *thread;
static struct task_monitor task; //tache a surveiller
static struct task_sample sample;


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
		if (!get_sample(&task, &sample))
			pr_warn("pid %d is not alive\n", process_id);
		else
			pr_info("pid %d usr %llu sys %llu", process_id,
				sample.utime, sample.stime);

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
	if (get_sample(&task, &sample))
		return snprintf(buff, PAGE_SIZE, "pid %d usr %llu sys %llu\n",
				process_id, sample.utime, sample.stime);
	else
		return snprintf(buff, PAGE_SIZE, "pid %d is not alive\n",
				process_id);
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
	pr_warn("exit\n");
}
module_exit(module_exit_taskmonitor);
