#include "dsm.h"

#include <stdio.h>
#include <sys/wait.h>

struct fibonacci{
    uint64_t N;
    uint64_t f0;
    uint64_t f1;
    uint64_t iter;
};

/**
 * f(n) = f(n-1) + f(n-2)
 */
void calcule_fibonacci(){
    printf("pid: %d se prépare\n",getpid());
    struct fibonacci *fibo;
    fibo = (struct fibonacci*) InitSlave("localhost");
    sleep(1);
    // Pour attendre que le maitre se mette en route
    while(1){
        lock_write(fibo,sizeof(fibo));
        if(fibo->N <= fibo->iter){
            printf("-------------------------\npid:%d\nf0:%lu\nf1:%lu\nN:%lu\niter:%lu\n------------------\n",getpid(),fibo->f0,fibo->f1,fibo->N,fibo->iter);
            unlock_write(fibo, sizeof(fibo));
            break;
        }
        //increment iteration
        fibo->iter++;
        //calcule
        uint64_t f0 = fibo->f0;
        uint64_t f1 = fibo->f1;
        uint64_t f2 = f0 + f1;
        //mise à jour struct fibonacci
        fibo->f0 = f1;
        fibo->f1 = f2;
        unlock_write(fibo, sizeof(fibo));
    }
    //destroy_slave();
}
void test_Fibonacci(void){
    if (fork() != 0) {
        struct fibonacci *fibo;
        fibo = (struct fibonacci*) InitMaster(sizeof(struct fibonacci));
        /* Initialisation du segment */
        fibo->N = 30;
        fibo->f0 = 0;
        fibo->f1 = 1;
        fibo->iter = 0;
        printf("En traitement ...\n");
        loop_master();
    } else {
        if(fork() != 0){
            calcule_fibonacci();
            wait(NULL);
        }else{
            if(fork()!=0){
                calcule_fibonacci();
                wait(NULL);
            }else{
                if(fork()!=0){
                    calcule_fibonacci();
                    wait(NULL);
                }else{
                    if(fork()!=0){
                        calcule_fibonacci();
                        wait(NULL);
                    }else{
                        calcule_fibonacci();
                        wait(NULL);
                    }
                }
            }
        }
    }
}

/**
 * calculer fibonacci avec 5 clients
 */
int main(){
    test_Fibonacci();
    return 0;
}