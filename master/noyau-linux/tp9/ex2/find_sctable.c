#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <asm/special_insns.h>

MODULE_DESCRIPTION("Searching for syscall table @");
MODULE_AUTHOR("Maxime Derri");
MODULE_LICENSE("GPL");

static long *sys_close_x86_64_addr = (void *)0xffffffff811f9da0;
static long *syscl_table_addr = 0;
static u32 mask = 10000;

static void find_sc_table_addr(void)
{
	unsigned long i = PAGE_OFFSET;
	unsigned long **addr;

	while (i < ULONG_MAX) { //from PAGE_OFFSET to last kernel address
		addr = (unsigned long **)i;
		if (addr[__NR_close] == sys_close_x86_64_addr) {
			pr_warn("addr of sys_close for x86_64: %lx     %lx\n",
					(unsigned long)sys_close_x86_64_addr,
					(unsigned long)addr[__NR_close]);
			syscl_table_addr = (void *)addr[0];
			pr_warn("sys_call_table[0]: %lx\n",
					(unsigned long)syscl_table_addr);
			return;
		}

		i += sizeof(void *);
	}
}

static int __init module_init_find_sctable(void)
{
	u32 old_cr0, cr0;

	find_sc_table_addr();

	pr_warn("init\n");
	pr_warn("changing cr0 bit\n");

	
	old_cr0 = native_read_cr0(); //sauvegarde
	cr0 = old_cr0;
	cr0 &= ~mask; //changement du bit 16 a 0
	native_write_cr0(cr0); //ecriture
	
	syscl_table_addr[0] = syscl_table_addr[0]; //ecriture pour test les droits
	
	native_write_cr0(old_cr0); //restautrer cr0
	

	pr_warn("change done\n");
	return 0;
}
module_init(module_init_find_sctable);

static void __exit module_exit_find_sctable(void)
{
	pr_warn("exit\n");
}
module_exit(module_exit_find_sctable);
