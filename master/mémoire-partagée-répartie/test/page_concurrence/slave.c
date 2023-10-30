#include <sys/wait.h>

#include "page_struct.h"
#include "dsm.h"

int main(){
    if(fork() != 0){
        struct page_struct *pages;
        pages = (struct page_struct*) InitSlave("localhost");

        //obtenir une page
        lock_read(pages,sizeof(struct page_struct)+PAGE_SIZE);
        printf("valeur : %c\n",pages->pages[0]);
        sleep(5);
        unlock_read(pages,sizeof(struct page_struct)+PAGE_SIZE);
        wait(NULL);
    }else{
        struct page_struct *pages;
        pages = (struct page_struct*) InitSlave("localhost");
        sleep(4);
        //obtenir 2 pages
        lock_read(pages,sizeof(struct page_struct)+2*PAGE_SIZE);
        printf("valeur : %c\n",pages->pages[2*PAGE_SIZE]);
        unlock_read(pages,sizeof(struct page_struct)+2*PAGE_SIZE);
    }
}