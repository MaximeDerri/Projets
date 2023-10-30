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

MODULE_DESCRIPTION("dentry discovering module");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

char *path = "/usr/bin"; //juste un truc par defaut, mais on doit passer $PATH
module_param(path, charp, 0600);

#define PATH_BUFF_SIZE 1000 //devrait pas arriver, pour avoir une grande marge
char path_buff[PATH_BUFF_SIZE+1];


static int weasel_list_show(struct seq_file *sf, void *p)
{
	struct hlist_bl_node *it;
	struct hlist_bl_head *head = dentry_hashtable;
	struct dentry *dent;
	u64 size = 1UL << (32 - d_hash_shift);
	u64 i = 0;

	for (i = 0; i < size; ++i) {
		hlist_bl_for_each_entry(dent, it, head, d_hash) {
			dget(dent);
			//if (dent->d_inode == NULL)
			//	continue;
			seq_printf(sf, "%s\n", dent->d_name.name);
			dput(dent);
		}
		++head;
	}
	return 0;
}

static int weasel_popen_list(struct inode *ino, struct file *f)
{
	pr_warn("procfs list open\n");
	return single_open(f, weasel_list_show, NULL);
	return 0;
}

static ssize_t weasel_pread(struct file *f, char *buf, size_t len, loff_t *of)
{
	char tmp[] = "I am a weasel!\n";

	return simple_read_from_buffer(buf, len, of, tmp, strlen(tmp));
}


static int weasel_pwd_show(struct seq_file *sf, void *p)
{
	struct dentry *dent, *it;
	struct file *f;
	struct path pth;
	u64 size = 0;
	u64 sz, sz2;
	u64 i = 0;
	u64 j = 0;

	if (path == NULL)
		return 0;
	size = strlen(path)+1;
	if (size > PATH_BUFF_SIZE)
		return 0;

	for (i = 0; i < size; ++i) {
		if (path[i] == ':' || path[i] == '\n' || path[i] == '\0') {
			if (i-j == 0) { // on a :\n\0
				goto next;
			}
			//parcour d'un repertoir
			sz = sizeof(char) * (i - j);
			if (sz + 1 > PATH_BUFF_SIZE + 1) //trop long
				goto next;

			//preparer l'ouverture du repertoire
			snprintf(path_buff, sz + 1, "%s", path+j);
			f = filp_open(path_buff, O_RDONLY | O_DIRECTORY, 0666);
			if (IS_ERR(f))
				goto next;

			dent = file_dentry(f);
			dget(dent);

			//recherche des dentry fils
			seq_printf(sf, "%s\n", path_buff);
			list_for_each_entry(it, &dent->d_subdirs, d_child) {
				dget(it);
				//trop long
				sz2 = strlen(it->d_name.name);
				if (sz2 + sz + 2 > PATH_BUFF_SIZE + 1)
					continue;

				//nom complet
				snprintf(path_buff + sz, sz2 + 2, "/%s",
							it->d_name.name);

				//si n'existe pas, on affiche
				if (kern_path(path_buff, LOOKUP_FOLLOW, &pth)
									!= 0)
					seq_printf(sf, "\t%s\n",
						it->d_name.name);
				dput(it);
			}

			dput(dent);
			filp_close(f, NULL);
		} else
			continue;
next:
		j = i+1;
	}
	seq_printf(sf, "\n\n$PATH = %s\n", path);
	return 0;
}

static int weasel_popen_pwd(struct inode *ino, struct file *f)
{
	pr_warn("procfs pwd open\n");
	return single_open(f, weasel_pwd_show, NULL);
	return 0;
}

static struct proc_ops pops = {
	.proc_open = weasel_popen_list,
	.proc_read = seq_read,
	.proc_release = single_release
};

static struct proc_ops pops2 = {
	.proc_read = weasel_pread
};

static struct proc_ops pops3 = {
	.proc_open = weasel_popen_pwd,
	.proc_read = seq_read,
	.proc_release = single_release
};

static struct proc_dir_entry *weasel_procfs;
static struct proc_dir_entry *whoami_file;
static struct proc_dir_entry *list_file;
static struct proc_dir_entry *pwd_file;

static int __init module_init_weasel(void)
{
	//j'ai retiré l'affichage de la question 2/3 car cela affiche trop de
	//choses sur le terminal. Cependant c'est tres simple a faire:
	//on parcours la liste comme pour weasel_show et on fait le max
	//pour chaque liste... et pour l'adresse, on met dans pr_info
	//"%p", dentry_hashtable.
	int ret = 0;

	if (path == NULL) {
		pr_warn("arg path is NULL, you must give $PATH\n");
		ret = -EINVAL;
		goto err;
	}

	weasel_procfs = proc_mkdir("weasel", NULL); // proc/weasel/
	if (weasel_procfs == NULL) {
		ret = -ENOMEM;
		pr_err("error - proc mkdir\n");
		goto err;
	}

	list_file = proc_create("list", 0, weasel_procfs, &pops);
	if (list_file == NULL) { // proc/weasel/list
		pr_err("error - proc create\n");
		goto err1;
	}

	whoami_file = proc_create("whoami", 0, weasel_procfs, &pops2);
	if (whoami_file == NULL) { // proc/weasel/whoami
		pr_err("error - proc create\n");
		goto err2;
	}

	pwd_file = proc_create("pwd", 0, weasel_procfs, &pops3);
	if (pwd_file == NULL) { // proc/weasel/whoami
		pr_err("error - proc create\n");
		goto err3;
	}

	pr_warn("init\n");
	return 0;
err3:
	proc_remove(pwd_file);
err2:
	proc_remove(list_file);
err1:
	proc_remove(weasel_procfs);
	ret = -ENOMEM;
err:
	return ret;
}
module_init(module_init_weasel);

static void __exit module_exit_weasel(void)
{
	proc_remove(list_file);
	proc_remove(whoami_file);
	proc_remove(pwd_file);
	proc_remove(weasel_procfs);
	pr_warn("exit\n");
}
module_exit(module_exit_weasel);
