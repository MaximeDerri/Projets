// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_DESCRIPTION("module that takes args \"HELLO WORLD\"");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

static char *whom = "empty";
module_param(whom, charp, 0660);

static int howmany;
module_param(howmany, int, 0660);

static void print_hello(void)
{
	int i = 0;

	while (i < howmany) {
		pr_info("(%d) Hello, %s\n", i, whom);
		++i;
	}
}

static void print_bye(void)
{
	pr_info("Goodbye, %s\n", whom);

}

/* -------------------------------- */

static int __init init_mod(void)
{
	print_hello();
	return 0;
}
module_init(init_mod);

static void __exit exit_mod(void)
{
	print_hello();
	print_bye();
}
module_exit(exit_mod);