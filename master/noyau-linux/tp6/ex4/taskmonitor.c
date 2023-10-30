// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/pid.h>
#include <linux/kernel_stat.h>
#include <linux/sched/task.h>

MODULE_DESCRIPTION("monitor CPU stat of one process module");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");


static pid_t process_id = 1;
module_param(process_id, int, 0660); //pid_t <-> int

struct task_monitor {
	struct pid *pid;
};

//structure qui rerefence le thread ("process descriptor")
//contient toutes les informations dont le noyau a besoin a propos de la tache
static struct task_struct *thread;
static struct task_monitor task; //tache a surveiller


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
	struct task_struct *task_of_pid = get_pid_task(task.pid, PIDTYPE_PID);

	if (task_of_pid == NULL)
		return -ESRCH;

	while (!kthread_should_stop() && pid_alive(task_of_pid)) {
		pr_info("pid %d usr %llu sys %llu", process_id,
			task_of_pid->utime, task_of_pid->stime);

		//sleep
		set_current_state(TASK_UNINTERRUPTIBLE);
		//HZ = nbr de ticks par sec + nbr de ++ pour jiffies par sec
		schedule_timeout(HZ);
	}

	put_task_struct(task_of_pid);
	put_pid(task.pid);
	task.pid = NULL;

	return 0;
}

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

	pr_info("init\n");
	return 0;
}
module_init(module_init_taskmonitor);

static void __exit module_exit_taskmonitor(void)
{
	if (thread)
		kthread_stop(thread);

	if (task.pid)
		put_pid(task.pid);

	pr_info("exit\n");
}
module_exit(module_exit_taskmonitor);
