#include "dsm.h"

struct test{
    int i;
};

int main(){
    struct test *test;
    test = (struct test*) InitMaster(sizeof(struct test));
    /* Initialisation du segment */
    test->i = 100;
    loop_master();
}