// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/ioctl.h>
#include <linux/string.h>

#include "interface.h"

#define BUFFER_SIZE 500

MODULE_DESCRIPTION("hellosysfs read module");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

static int major;
static char *s = "Hello ";
static int base_size;
static char *def = "helloioctl\n";


static int hello_open(struct inode *ino, struct file *f)
{
	pr_warn("open helloioctl\n");

	f->private_data = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (f->private_data == NULL)
		return -ENOMEM;

	base_size = strlen(s);
	strcpy((char *)(f->private_data), s);
	strcat((char *)(f->private_data + base_size), def);
	return 0;
}

static int hello_release(struct inode *ino, struct file *f)
{
	pr_warn("release helloioctl\n");
	kfree(f->private_data);
	return 0;
}

static long hello_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	pr_debug("in ioctl\n");
	if (_IOC_TYPE(cmd) != MAGIC)
		return -EINVAL;

	switch (cmd) {
	case HELLOIOCG: //get
		if (copy_to_user((char *)arg, (char *)(f->private_data),
				strlen((char *)(f->private_data))) != 0)
			return -EINVAL;
		break;

	case HELLOIOCS: //set
		int i;
		int max = BUFFER_SIZE - base_size - 1; // -1 pour le \0
		char ch;
		char *tmp = (char *)arg;

		get_user(ch, tmp);
		for (i = 0; ch && i < max; ++i, ++tmp)
			get_user(ch, tmp);

		if (copy_from_user(((char *)(f->private_data)) + base_size,
				(char *)arg, i) != 0)
			return -EINVAL;

		((char *)(f->private_data))[base_size + i] = '\0';

		break;

	default:
		return -ENOTTY;
	}
	return 0;
}


static const struct file_operations fops = {
	.open = hello_open,
	.release = hello_release,
	.unlocked_ioctl = hello_ioctl,
};

static int __init module_init_helloioctl(void)
{
	major = register_chrdev(0, "hello", &fops);

	pr_warn("init\n");
	return 0;
}
module_init(module_init_helloioctl);

static void __exit module_exit_helloioctl(void)
{
	unregister_chrdev(major, "hello");
	pr_warn("exit\n");
}
module_exit(module_exit_helloioctl);
