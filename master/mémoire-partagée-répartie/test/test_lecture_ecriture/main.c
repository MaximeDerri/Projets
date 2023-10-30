#include <stdio.h>
//#include <fics>.h

//interface DSM file
#include "dsm.h"

// Meme test que le prof pour voir deja 
struct seg_part {
    int tab[1200];
    int a;
};

/*
    Simple test de lecture ecriture entre 4 processus esclaves
*/
int main(void) {
     if (fork() != 0) {
        struct seg_part *seg;
        seg = (struct seg_part*) InitMaster(sizeof(struct seg_part));

        /* Initialisation du segment */
        seg->a = 10;
        for (int i=0; i < 1200; i++)
            seg->tab[i] = i;

        LoopMaster();
    } else {
        // Pour attendre que le maitre se mette en route
        sleep(2);

        if (fork() != 0) {

            if (fork() != 0) {
                struct seg_part *seg4;
                seg4 = (struct seg_part*) InitSlave("localhost");
                sleep(3);

                lock_read(&(seg4->a), sizeof(seg4->a));
                printf("a = %d\n", seg4->a);
                unlock_read(&(seg4->a), sizeof(seg4->a));
            
                printf("fin proc 1\n");
            } else {
                struct seg_part *seg1;
                seg1 = (struct seg_part*) InitSlave("localhost");
                sleep(3);
                lock_write(&(seg1->a), sizeof(seg1->a));
                printf("a = %d\n", seg1->a);
                seg1->a = 5;
                printf("a = %d\n", seg1->a);
                unlock_write(&(seg1->a), sizeof(seg1->a));

                printf("fin proc 2\n");
            }
        } else {
            if (fork() != 0) {
                struct seg_part *seg3;
                seg3 = (struct seg_part*) InitSlave("localhost");
                sleep(3);

                lock_read(&(seg3->a), sizeof(seg3->a));
                printf("a = %d\n", seg3->a);
                unlock_read(&(seg3->a), sizeof(seg3->a));
               
                printf("fin proc 3\n");
            } else {
                struct seg_part *seg2;
                seg2 = (struct seg_part*) InitSlave("localhost");
                sleep(3);

                lock_read(&(seg2->a), sizeof(seg2->a));
                printf("a = %d\n", seg2->a);
                unlock_read(&(seg2->a), sizeof(seg2->a));
                printf("fin proc 4\n");
            }
        }
    }
}