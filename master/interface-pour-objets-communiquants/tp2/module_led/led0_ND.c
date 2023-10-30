#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <mach/platform.h>
#include <asm/uaccess.h>


#define NBMAX_LED 32

#define GPIO_BP     18 /* on pourait le mettre dans les parametres du module */
#define LED0 4         /* a mettre dans les param avec led=4 */

#define GPIO_FSEL_INPUT  0
#define GPIO_FSEL_OUTPUT 1

struct gpio_s
{
    uint32_t gpfsel[7];
    uint32_t gpset[3];
    uint32_t gpclr[3];
    uint32_t gplev[3];
    uint32_t gpeds[3];
    uint32_t gpren[3];
    uint32_t gpfen[3];
    uint32_t gphen[3];
    uint32_t gplen[3];
    uint32_t gparen[3];
    uint32_t gpafen[3];
    uint32_t gppud[1];
    uint32_t gppudclk[3];
    uint32_t test[1];
}
volatile *gpio_regs = (struct gpio_s *)__io_address(GPIO_BASE);

MODULE_LICENSE       ("GPL");
MODULE_AUTHOR        ("Normand_Derri, 2023");
MODULE_DESCRIPTION   ("Module gérant une LED");

static int major;
static int led;

module_param(led, int, 0);
MODULE_PARM_DESC(led, "numero de port de la LED");

static void gpio_fsel(int pin, int fun)
{
    uint32_t reg = pin / 10;
    uint32_t bit = (pin % 10) * 3;
    uint32_t mask = 0b111 << bit;
    gpio_regs->gpfsel[reg] = (gpio_regs->gpfsel[reg] & ~mask) | ((fun << bit) & mask);
}

static void gpio_write(int pin, bool val)
{
    if (val)
        gpio_regs->gpset[pin / 32] = (1 << (pin % 32));
    else
        gpio_regs->gpclr[pin / 32] = (1 << (pin % 32));
}

/* pour BP */
static int
gpio_read(uint32_t pin) {
    uint32_t reg = pin/32;
    uint32_t bit= pin%32;
    return gpio_regs->gplev[reg] & (1 << bit);
}


static int  open_ND(struct inode *inode, struct file *file) {
    printk(KERN_DEBUG "open()\n");
    gpio_fsel(GPIO_BP, GPIO_FSEL_INPUT);
    gpio_fsel(led, GPIO_FSEL_OUTPUT);
    return 0;
}

static ssize_t read_bp_ND(struct file *file, char *buf, size_t count, loff_t *ppos) {
    int rec;
    char c;
    /* on ne peut pas acceder à pthread.h et time.h, sinon on aurait implémenté le même comportement qu'en TP1 */
    /* avec BP_ON et un thread, qui attend 20ms pour verifier le bouton poussoir */
    rec = gpio_read(GPIO_BP);
    printk(KERN_DEBUG "read from BP = %d\n", rec);
    if(rec)
    {
        c = '1';
        rec = copy_to_user(buf, &c, 1);
    }
    else
    {
        c = '0';
        rec = copy_to_user(buf, &c, 1);
    }
    return count;

}

static ssize_t write_led_ND(struct file *file, const char *buf, size_t count, loff_t *ppos) {
    char c;
    printk(KERN_DEBUG "write()\n");
    c = *buf;
    if(c == '0') {
        gpio_write(led, 0);
    }
    else {
        gpio_write(led, 1);
    }
    return count;
}

static int release_ND(struct inode *inode, struct file *file) {
    printk(KERN_DEBUG "close()\n");
    gpio_write(led, 0);
    return 0;
}

struct file_operations fops_led =
{
    .open       = open_ND,
    .read       = read_bp_ND,
    .write      = write_led_ND,
    .release    = release_ND 
};

static int __init mon_module_init(void)
{
   printk(KERN_DEBUG "Hello World <Normand_Derri> !\n");
   major = register_chrdev(0, "led0_ND", &fops_led); 
   return 0;
}

static void __exit mon_module_cleanup(void)
{
   printk(KERN_DEBUG "Goodbye World!\n");
   unregister_chrdev(major, "led0_ND");
}

module_init(mon_module_init);
module_exit(mon_module_cleanup);