#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <mach/platform.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "lcd_ND.h"

volatile struct gpio_s *gpio_regs = (struct gpio_s *)__io_address(GPIO_BASE);
/* static int      line_length       = 20; Longueur de base = 20  */

/* module_param(line_length, int, 0); */


MODULE_LICENSE       ("GPL");
MODULE_AUTHOR        ("Normand_Derri, 2023");
MODULE_DESCRIPTION   ("Module gérant une LCD");
/* MODULE_PARAM_DESC    (line_length, "Défini la longueur d'une ligne de l'ecran LCD"); */


static int major;

/**************************************************/
void gpio_config(int gpio, int value)
{
    int regnum = gpio / 10;
    int offset = (gpio % 10) * 3;
    gpio_regs->gpfsel[regnum] &= ~(0x7 << offset);
    gpio_regs->gpfsel[regnum] |= ((value & 0x7) << offset);
}

void gpio_write(int gpio, int value)
{
    int regnum = gpio / 32;
    int offset = gpio % 32;
    if (value&1)
        gpio_regs->gpset[regnum] = (0x1 << offset);
    else
        gpio_regs->gpclr[regnum] = (0x1 << offset);
}

/* generate E signal */
void lcd_strobe(void)
{
    gpio_write(E, 1);
    udelay(20);
    gpio_write(E, 0);
}

/* send 4bits to LCD : valable pour les commande et les data */
void lcd_write4bits(int data)
{   
    printk(KERN_DEBUG ":   %c\n", (char)data);
    /* first 4 bits */
    gpio_write(D7, data>>7);
    gpio_write(D6, data>>6);
    gpio_write(D5, data>>5);
    gpio_write(D4, data>>4);
    lcd_strobe();

    /* second 4 bits */
    gpio_write(D7, data>>3);
    gpio_write(D6, data>>2);
    gpio_write(D5, data>>1);
    gpio_write(D4, data>>0);
    lcd_strobe();
}

void lcd_command(int cmd)
{
    gpio_write(RS, 0);
    lcd_write4bits(cmd);
    udelay(2000);               // certaines commandes sont lentes 
}


void lcd_data(int character)
{
    gpio_write(RS, 1);
    lcd_write4bits(character);
    udelay(20);
}

void lcd_set_cursor (struct file* file, int x, int y)
{
    struct pos_cursor * curseur = file->private_data; 
    int a[] = { 0, 0x40, 0x14, 0x54 };
    int i;

    /* Mise à jour de la struct */
    curseur->column = x;
    curseur->line   = y;


    /* Reel mise à jour du curseur */
    lcd_command(LCD_RETURNHOME);
    lcd_command(LCD_SETDDRAMADDR + a[y%4]);

    for(i = 0; i<x%20; ++i){
        lcd_command(LCD_CURSORSHIFT | LCD_CS_MOVERIGHT);
    }

}

/**
 * @brief Retourne au début de la ligne courante
 *        Met à jour la structure pos_cursor
 * 
 * @param file 
 */
inline void lcd_cursor_linestart(struct file* file){
    struct pos_cursor * curseur = file->private_data; 

    lcd_set_cursor(file, 0, curseur->line);
}

/**
 * @brief Positionne le curseur à la colonne zéro de la ligne suivante modulo 4.
 *        Mets à jour la structure pos_cursor. 
 * 
 * @param file 
 */
inline void lcd_cursor_nextline(struct file* file){
    struct pos_cursor * curseur = file->private_data; 

    lcd_set_cursor(file, 0, curseur->line+1);
}


/**
 * @brief Affiche un message sur l'ecran LCD
 * 
 * @param message Message à afficher
 * @param file    associé à notre open, pour récupérer notre struct pos_cursor
 */
void lcd_print_2(const char* message, struct file* file)
{
    struct pos_cursor* curseur = file->private_data;
    int message_size = strlen(message);
    int i;

    printk(KERN_DEBUG "lcd_print(): reçu: %s\n");

    for (i = 0; i < message_size; i++ ){

        if( message[i] == '\n' ){
            lcd_cursor_nextline(file);
            continue;
        }

        if( message[i] == '\r' ){
            lcd_cursor_linestart(file);
            continue;
        }

        if( curseur->column == 20 )
            lcd_cursor_nextline(file);

        lcd_data(message[i]);
        curseur->column = curseur->column+1;

    }
}


/**
 *  Pas de prise en charge de la structure de curseur globale ?
 *  Marche surement mais sans qu'on en comprendre les mécanismes fondamentaux
 *  A revoir pour une meilleur lisibilité et compatibilité avec nôtre module... 
 */
void lcd_print(const char *txt, struct file* file)
{   
    int len = 20;
    int i, l, lf;
    lf = 0;

    lcd_set_cursor(file, 0, 0);
    for (i = 0, l = 0; (l < 4) && (i < strlen(txt)-1); l++) {
        for (; (i < (l + 1) * len) && (i < strlen(txt)-1); i++) {
            if (txt[i] == '\n') {
                ++l;
                lcd_cursor_nextline(file);
            }
            else if(txt[i] == '\r') {
                lcd_cursor_linestart(file);
                printk(KERN_DEBUG "else if\n" );
            }
            else {
                lcd_data(txt[i]);
            }
        }
        lcd_cursor_nextline(file);
    }
}

void lcd_init(void)
{
    gpio_write(E, 0);
    lcd_command(0b00110011);    /* initialization */
    lcd_command(0b00110010);    /* initialization */
    lcd_command(LCD_FUNCTIONSET | LCD_FS_4BITMODE | LCD_FS_2LINE | LCD_FS_5x8DOTS);
    lcd_command(LCD_DISPLAYCONTROL | LCD_DC_DISPLAYON | LCD_DC_CURSORON | LCD_DC_BLINKON);
    lcd_command(LCD_ENTRYMODESET | LCD_EM_RIGHT | LCD_EM_DISPLAYNOSHIFT);
}

void lcd_clear(void)
{
    lcd_command(LCD_CLEARDISPLAY);
    lcd_command(LCD_RETURNHOME);
}

static long ioctl_lcd(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct pos_cursor pos;
    
    printk(KERN_DEBUG "Ioctl_lcd ! \n");
    
    if(_IOC_TYPE(cmd) != IOC_MAGIC) // Check the magic number of the device
        return -EINVAL;
    
    switch(cmd){
        case LCDIOCT_CLEAR:
            file->f_pos = 0;
            lcd_clear();
            lcd_set_cursor(file, 0, 0);
            break;

        case LCDIOCT_SETXY:
            if(copy_from_user(&pos, (struct pos_cursor*)arg, _IOC_SIZE(cmd)) != 0)
                return -EINVAL;

            lcd_set_cursor(file, pos.column, pos.line);
            break;

        default: return -EINVAL;
    }
    
    return 0;
}


/**************************************************/



static int  open_ND(struct inode *inode, struct file *file) {
    printk(KERN_DEBUG "open()\n");
    file->private_data = (struct pos_cursor *)kmalloc(sizeof(struct pos_cursor), GFP_KERNEL);
    lcd_set_cursor(file, 0, 0);

    return 0;
}

static ssize_t write_lcd_ND(struct file *file, const char *buf, size_t count, loff_t *ppos) {
    char c[count+1];

    if (copy_from_user(c, buf, count) < 0) {
        printk(KERN_DEBUG "write() error copy_from_user()\n");
        return -1;
    }
    printk(KERN_DEBUG "write() %d\n", count);
    c[count] = '\0';
    printk(KERN_DEBUG "msg: %s\n", c);
    lcd_print_2(c, file);
    return count;
}

static int release_ND(struct inode *inode, struct file *file) {
    printk(KERN_DEBUG "close()\n");

/*
    udelay(2000);
    udelay(2000);
    udelay(2000);
    lcd_print("closing...", file);
    udelay(2000);
    lcd_clear();
*/

    kfree(file->private_data);
    return 0;
}

struct file_operations fops_lcd =
{
    .open            = open_ND,
    .write           = write_lcd_ND,
    .unlocked_ioctl  = ioctl_lcd,
    .release         = release_ND 
};

static int __init mod_init(void)
{
    printk(KERN_DEBUG "Hello World <Normand_Derri> !\n");
    major = register_chrdev(0, "lcd_ND", &fops_lcd); 
    lcd_init();
    lcd_clear();

    gpio_config(RS, GPIO_OUTPUT);
    gpio_config(E,  GPIO_OUTPUT);
    gpio_config(D4, GPIO_OUTPUT);
    gpio_config(D5, GPIO_OUTPUT);
    gpio_config(D6, GPIO_OUTPUT);
    gpio_config(D7, GPIO_OUTPUT);
    
    return 0;
}

static void __exit mod_cleanup(void)
{
   printk(KERN_DEBUG "Goodbye World!\n");
   unregister_chrdev(major, "lcd_ND");
   lcd_clear();
}


module_init(mod_init);
module_exit(mod_cleanup);