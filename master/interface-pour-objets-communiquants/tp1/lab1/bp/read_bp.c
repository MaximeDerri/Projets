#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include "thread_param.h"

//------------------------------------------------------------------------------
// GPIO ACCES
//------------------------------------------------------------------------------

#define BCM2835_PERIPH_BASE     0x20000000
#define BCM2835_GPIO_BASE       ( BCM2835_PERIPH_BASE + 0x200000 )

#define GPIO_LED0   4
#define GPIO_LED1   17
#define GPIO_BP     18

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
};

struct gpio_s *gpio_regs_virt;

uint32_t BP_ON = 0;
uint32_t BP_OFF = 0;

pthread_t th1, th2, th3;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

struct pt_param th2p;
struct pt_param th3p;

static void 
gpio_fsel(uint32_t pin, uint32_t fun)
{
    uint32_t reg = pin / 10;
    uint32_t bit = (pin % 10) * 3;
    uint32_t mask = 0b111 << bit;
    gpio_regs_virt->gpfsel[reg] = (gpio_regs_virt->gpfsel[reg] & ~mask) | ((fun << bit) & mask);
}

static void 
gpio_write (uint32_t pin, uint32_t val)
{
    uint32_t reg = pin / 32;
    uint32_t bit = pin % 32;
    if (val == 1) 
        gpio_regs_virt->gpset[reg] = (1 << bit);
    else
        gpio_regs_virt->gpclr[reg] = (1 << bit);
}

//------------------------------------------------------------------------------
// Access to memory-mapped I/O
//------------------------------------------------------------------------------

#define RPI_PAGE_SIZE           4096
#define RPI_BLOCK_SIZE          4096

static int mmap_fd;

static int
gpio_mmap ( void ** ptr )
{
    void * mmap_result;

    mmap_fd = open ( "/dev/mem", O_RDWR | O_SYNC );

    if ( mmap_fd < 0 ) {
        return -1;
    }

    mmap_result = mmap (
        NULL
      , RPI_BLOCK_SIZE
      , PROT_READ | PROT_WRITE
      , MAP_SHARED
      , mmap_fd
      , BCM2835_GPIO_BASE );

    if ( mmap_result == MAP_FAILED ) {
        close ( mmap_fd );
        return -1;
    }

    *ptr = mmap_result;

    return 0;
}

void
gpio_munmap ( void * ptr )
{
    munmap ( ptr, RPI_BLOCK_SIZE );
}

//------------------------------------------------------------------------------
// Main Programm
//------------------------------------------------------------------------------

void
delay ( unsigned int milisec )
{
    struct timespec ts, dummy;
    ts.tv_sec  = ( time_t ) milisec / 1000;
    ts.tv_nsec = ( long ) ( milisec % 1000 ) * 1000000;
    nanosleep ( &ts, &dummy );
}

static int
gpio_read(uint32_t pin) {
    uint32_t reg = pin/32;
    uint32_t bit= pin%32;
    return gpio_regs_virt->gplev[reg] & (1 << bit);
}

void *run_bp(void *rien)
{
    uint32_t val_prec = 1;
    uint32_t val_nouv = 1;

    while(1) {
        delay(20);
        val_nouv = gpio_read(GPIO_BP);
        if(val_prec != val_nouv) {
            if(val_nouv == 0) {
                pthread_mutex_lock(&mtx);
                BP_ON = 1;
                pthread_mutex_unlock(&mtx);
            }
            else {
                pthread_mutex_lock(&mtx);
                BP_OFF = 1;
                BP_ON  = 0;
                pthread_mutex_unlock(&mtx);
            }
        }
        val_prec = val_nouv;
    }
    pthread_exit(0);
}

void* led_handling (void* args) 
{
    struct pt_param* param = (struct pt_param *) args;
    int state = 0;
    int tmp = 0;
    while ( 1 ){
        pthread_mutex_lock(&mtx);
        tmp = BP_ON;
        pthread_mutex_unlock(&mtx);
        if(tmp == 1 ){
            if(state){
                gpio_write ( param->id, state );
                state = 0;
            }else{
                gpio_write ( param->id, state );
                state = 1;
            }     
        }
    }
    pthread_exit(0);
}

void* blink (void *p)
{
    uint32_t val = 0;
    struct pt_param * param = (struct pt_param *)p;

    while (1) {
        gpio_write ( param->id, val );
        delay ( param->hald_period);
        val = 1 - val;
    }

    pthread_exit(0);
}


int main(int argc, char *argv[]) {
    // Get args
    // ---------------------------------------------

    int period, half_period;

    period = 1000; /* default = 1Hz */
    if ( argc > 1 ) {
        period = atoi ( argv[1] );
    }
    uint32_t volatile * gpio_base = 0;

    // map GPIO registers
    // ---------------------------------------------

    if ( gpio_mmap ( (void **)&gpio_regs_virt ) < 0 ) {
        printf ( "-- error: cannot setup mapped GPIO.\n" );
        exit ( 1 );
    }

    // Setup GPIO of BP to input
    // ---------------------------------------------
    gpio_fsel(GPIO_BP, GPIO_FSEL_INPUT);


    if(pthread_create(&th1, NULL, run_bp, (char *)NULL) < 0) {
        exit(1);
    }

    th2p.hald_period = period; 
    th2p.id = GPIO_LED1;
    if(pthread_create(&th2, NULL, blink, (void *) &th2p) < 0) {
        exit(1);
    }

    th3p.hald_period = period; 
    th3p.id = GPIO_LED0;
    if(pthread_create(&th3, NULL, led_handling, (void *) &th3p) < 0) {
        exit(1);
    }



    while(1); //{
//        if(BP_ON == 1 ) {
//            BP_ON = 0;
//        }
//    }

    return 0;
}