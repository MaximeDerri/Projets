// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/kobject.h>

MODULE_DESCRIPTION("Module \"hide module\"");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

static int __init hello_init(void)
{
	pr_info("let's hide !!!\n");
	list_del(&THIS_MODULE->list);

	kobject_del(&THIS_MODULE->mkobj.kobj);
	list_del(&THIS_MODULE->mkobj.kobj.entry);
	pr_info("still alive and I disapear\n");
	return 0;
}
module_init(hello_init);

static void __exit hello_exit(void)
{
	pr_info("Goodbye\n");
}
module_exit(hello_exit);

