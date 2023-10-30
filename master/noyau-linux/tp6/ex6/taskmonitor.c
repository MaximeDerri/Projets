// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/pid.h>
#include <linux/kernel_stat.h>
#include <linux/sched/task.h>
#include <linux/fs.h>
#include <linux/uacces.h>
#include <linux/mutex.h>
#include <asm/ioctl.h>

#include "interface_tm.h"

MODULE_DESCRIPTION("monitor CPU stat of one process module");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");


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
static int major;
DEFINE_MUTEX(mutex);


static s8 get_sample(struct task_monitor *tm, struct task_sample *sample)
{
	struct task_struct *task_of_pid;

	if (tm->pid == NULL) //si on est dans le moment du changement de pid
		return 0;

	mutex_lock(&mutex);
	task_of_pid = get_pid_task(tm->pid, PIDTYPE_PID);
	if (task_of_pid == NULL)
		goto not_alive;
	if (pid_alive(task_of_pid) == 0) {
		put_task_struct(task_of_pid);
		goto not_alive;
	}

	sample->utime = task_of_pid->utime;
	sample->stime = task_of_pid->stime;
	put_task_struct(task_of_pid);
	mutex_unlock(&mutex);
	return 1;

not_alive:
	mutex_unlock(&mutex);
	return 0;
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

static int taskmonitor_open(struct inode *ino, struct file *f)
{
	pr_warn("open\n");
	return 0;
}

static int taskmonitor_release(struct inode *ino, struct file *f)
{
	pr_warn("release\n");
	return 0;
}

static long taskmonitor_ioctl(struct file *f, unsigned int cmd,
			unsigned long arg)
{
	if (_IOC_TYPE(cmd) != MAGIC)
		return -EINVAL;

	switch (cmd) {
	case TASKMONITORIOCG_BUFFER:
		char tmp_buff[BUFF_SIZE];

		if (get_sample(&task, &sample))
			snprintf(tmp_buff, BUFF_SIZE,
				"pid %d usr %llu sys %llu",
				process_id, sample.utime, sample.stime);
		else
			snprintf(tmp_buff, BUFF_SIZE,
				"pid %d is not alive", process_id);
		//envoie vers usr
		if (copy_to_user((char *)arg, tmp_buff,
				(strlen(tmp_buff) + 1)) != 0)
			return -EINVAL;
		break;
	case TASKMONITORIOCG_STRUCT:
		struct task_sample tmp_ts;

		if (get_sample(&task, &sample)) {
			tmp_ts.utime = sample.utime;
			tmp_ts.stime = sample.stime;
		} else {
			pr_warn("pid %d is not alive\n", process_id);
			tmp_ts.utime = 0;
			tmp_ts.stime = 0;
		}

		if (copy_to_user((struct task_sample *)arg, &tmp_ts,
				sizeof(struct task_sample)) != 0)
			return -EINVAL;
		break;
	case TASKMONITORIOCT_STOP:
		if (thread) { //le thread existe
			kthread_stop(thread);
			thread = NULL;
			pr_warn("thread was killed\n");
		} else
			pr_warn("thread is already dead\n");
		break;
	case TASKMONITORIOCT_START:
		if (!thread) { //si le thread est en cours, on en relance pas
			thread = kthread_run(monitor_fn, NULL, "monitor_fn");
			if (IS_ERR(thread)) {
				pr_warn("kthread_run error - start msg: %ld\n",
					PTR_ERR(thread));
			}
			pr_warn("thread was created\n");
		} else
			pr_warn("thread is already running\n");
		break;
	case TASKMONITORIOCT_SETPID:
		pid_t new_pid;
		long ret;

		if (copy_from_user(&new_pid, (pid_t *)arg, sizeof(pid_t)) != 0)
			return -EINVAL;

		mutex_lock(&mutex);
		if (task.pid) {
			put_pid(task.pid);
			task.pid = NULL;
			pr_warn("put pid ref\n");
		}


		process_id = new_pid;
		sample.utime = 0;
		sample.stime = 0;
		pr_warn("reset sample and set new pid\n");
		ret = monitor_pid(process_id); //0 or -ESRCH

		mutex_unlock(&mutex);
		return ret;
	default:
		return -ENOTTY;
	}

	return 0;
}

static const struct file_operations fops = {
	.open = taskmonitor_open,
	.release = taskmonitor_release,
	.unlocked_ioctl = taskmonitor_ioctl,
};

static int __init module_init_taskmonitor(void)
{
	int ret = 0;

	ret = monitor_pid(process_id); //recuperer la struct pid
	if (ret)
		return ret;

	//thread noyau
	thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	if (IS_ERR(thread)) {
		put_pid(task.pid);
		return PTR_ERR(thread);
	}

	mutex_init(&mutex);
	major = register_chrdev(0, "taskmonitor", &fops);

	pr_warn("init\n");
	return 0;
}
module_init(module_init_taskmonitor);

static void __exit module_exit_taskmonitor(void)
{
	unregister_chrdev(major, "taskmonitor");

	if (thread)
		kthread_stop(thread);

	if (task.pid)
		put_pid(task.pid);

	pr_warn("exit\n");
}
module_exit(module_exit_taskmonitor);
