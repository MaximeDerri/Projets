#include "page_struct.h"
#include "dsm.h"

int main(){
    struct page_struct *pages;
    pages = (struct page_struct*) InitMaster(sizeof(struct page_struct) + 3*PAGE_SIZE);
    /* Initialisation du segment */
    pages->taille = 3*PAGE_SIZE;

    pages->pages[0] = '2';
    pages->pages[PAGE_SIZE] = '0';
    pages->pages[2*PAGE_SIZE] = '1';

    printf("master pages[0]:%c\npages[PAGE_SIZE]:%c\npages[2*PAGE_SIZE]:%c\n",pages->pages[0],pages->pages[PAGE_SIZE],pages->pages[2*PAGE_SIZE]);
    LoopMaster();
}