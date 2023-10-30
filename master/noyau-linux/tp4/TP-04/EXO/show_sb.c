// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

MODULE_DESCRIPTION("module that show SB infos");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

static void dump_sb(struct super_block *sb, void *arg)
{
	pr_info("uuid=%pUb type=%s\n", sb->s_uuid.b, sb->s_type->name);
}


static int __init init_mod(void)
{
	pr_info("init show_sb\n");
	iterate_supers(dump_sb, NULL);
	return 0;
}
module_init(init_mod);

static void __exit exit_mod(void)
{
	pr_info("exit show_sb\n");
}
module_exit(exit_mod);


