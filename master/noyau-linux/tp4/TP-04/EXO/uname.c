// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/string.h>


MODULE_DESCRIPTION("version \"uname module\"");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

#define LOCAL_UTS_LEN (__NEW_UTS_LEN + 1)
static char save[LOCAL_UTS_LEN];


static char *name = "";
module_param(name, charp, 0600);


static int save_sysname(void)
{
	int ret = 0;

	memset(save, (char)'\0', LOCAL_UTS_LEN);
	ret = strlen(init_uts_ns.name.sysname);
	if (ret < 0) {
		pr_info("error from strlen in save_system() - uname.ko\n");
		return ret;
	}

	/* saving real name */
	memcpy(save, init_uts_ns.name.sysname, ret);
	return 0;
}

static int alter_sysname(const char *src)
{
	if (strlen(src) > LOCAL_UTS_LEN) {
		pr_info("bad src length in alter_sysname() - uname.ko\n");
		return -1;
	}

	memcpy(init_uts_ns.name.sysname, src, LOCAL_UTS_LEN);
	return 0;
}

/* ------------------------- */

static int __init init_mod(void)
{
	if (save_sysname() < 0)
		return -1;

	if (alter_sysname(name) < 0)
		return -1;

	pr_info("init mod uname init, name set to: %s\n", name);
	return 0;
}
module_init(init_mod);

static void __exit exit_mod(void)
{
	alter_sysname(save);
	pr_info("init mod uname end - original name has been restored\n");
}
module_exit(exit_mod);
