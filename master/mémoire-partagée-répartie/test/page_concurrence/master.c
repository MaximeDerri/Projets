#include "page_struct.h"
#include "dsm.h"

int main(){
    struct page_struct *pages;
    pages = (struct page_struct*) InitMaster(sizeof(struct page_struct) + 3*PAGE_SIZE);
    pages->taille = 3*PAGE_SIZE;
    printf("taille page: %d, PAGE_SIZE: %ld\n",pages->taille,PAGE_SIZE);
    /* Initialisation du segment */
    pages->pages[0]= 'A';
    pages->pages[2*PAGE_SIZE] = 'B';
    LoopMaster();
}