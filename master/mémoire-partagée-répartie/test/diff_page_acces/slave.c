#include "page_struct.h"
#include "dsm.h"

#include <sys/wait.h>

void getPlace(int initPos){
    int position;
    struct page_struct *pages;
    pages = (struct page_struct*) InitSlave("localhost");

    //obtenir une page, chaque page a une valeur défini par master
    lock_write(&pages->pages[initPos], PAGE_SIZE - 1);
    position = atoi(&pages->pages[initPos]);
    printf("PID:%d, valeur observé: %d\n",getpid(),position);
    pages->pages[initPos] = 'F';
    sleep(2);
    unlock_write(&pages->pages[initPos], PAGE_SIZE - 1);
}

int main(){
    if(fork() != 0){
        getPlace(0);
        wait(NULL);
    } else{
        if(fork() != 0){
            getPlace(PAGE_SIZE);
            wait(NULL);
        }else{
            getPlace(2*PAGE_SIZE);
        }
    } 
}