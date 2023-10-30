// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>

MODULE_DESCRIPTION("hellosysfs read module");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

static struct kobject *object;
static char rec_from_user[PAGE_SIZE];


static ssize_t hello_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buff)
{
	return snprintf(buff, PAGE_SIZE, "Hello %s!\n",
			rec_from_user[0] == '\0' ? "sysfs" : rec_from_user);
}

static ssize_t hello_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buff, size_t count)
{
	int rec = sscanf(buff, "%s", rec_from_user);

	if (count > PAGE_SIZE || rec <= 0) {
		rec_from_user[0] = '\0';
		return -EINVAL;
	}

	return count; //taille des donnees dans le buffer
}

static struct kobj_attribute object_attribute = __ATTR_RW(hello);

static int __init module_init_hellosysfs(void)
{
	int ret;

	ret = sysfs_create_file(kernel_kobj, &object_attribute.attr);
	if (ret) //rate donc pas besoin de le liberer
		return -ENOMEM;

	pr_warn("init module\n");
	return 0;

	/*
	object = kobject_create_and_add("hello", kernel_kobj);
	if (!object)
		goto err_init_1;

	ret = sysfs_create_file(object, &object_attribute.attr);
	if (ret)
		goto err_init_2;

	pr_info("init module\n");
	return 0;

err_init_2:
	kobject_put(object);
err_init_1:
	return -ENOMEM;
	*/
}
module_init(module_init_hellosysfs);

static void __exit module_exit_hellosysfs(void)
{
	//kobject_put(object);
	sysfs_remove_file(kernel_kobj, &object_attribute.attr);
	pr_warn("exit module\n");
}
module_exit(module_exit_hellosysfs);
