// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/timekeeping.h>

MODULE_DESCRIPTION("module that show SB infos");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

#define _10_POW_9 1000000000 /* 10^9 */

static char *type = "";
module_param(type, charp, 0600);


static void dump_sb(struct super_block *sb, void *arg)
{
	u64 new_time = ktime_get();

	pr_info("uuid=%pUb type=%s time=%llu.%llu\n", sb->s_uuid.b, sb->s_type->name, sb->sec_tp4, sb->msec_tp4);
	sb->sec_tp4 = (u64)(new_time / _10_POW_9);
	sb->msec_tp4 = (u64)(new_time - (sb->sec_tp4 * _10_POW_9));
}


static int __init init_mod(void)
{
	struct file_system_type *fst;

	pr_info("init show_sb\n");
	fst = get_fs_type(type);
	if (fst == NULL) {
		pr_info("get_gs_type() has returned NULL - no action will be performed\n");
		return -1;
	}
	iterate_supers_type(fst, dump_sb, NULL);

	/* release */
	put_filesystem(fst);
	return 0;
}
module_init(init_mod);

static void __exit exit_mod(void)
{
	pr_info("exit show_sb\n");
}
module_exit(exit_mod);


