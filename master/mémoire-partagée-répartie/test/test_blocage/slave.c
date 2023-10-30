//
// Created by changrui on 20/05/23.
//

#include "dsm.h"

struct test{
    int i;
};

int main(){
    if(fork()!=0){
        printf("je lance :%d\n",getpid());
        struct fibonacci *test;
        test = (struct fibonacci*) InitSlave("localhost");
        lock_read(test,sizeof(test));
        unlock_read(test, sizeof(test));
        printf("pid:%d end\n",getpid());
    }else{
        if(fork()!=0){
            printf("je lance :%d\n",getpid());
            struct fibonacci *test;
            test = (struct fibonacci*) InitSlave("localhost");
            lock_read(test,sizeof(test));
            unlock_read(test, sizeof(test));
            printf("pid:%d end\n",getpid());
        }else{
            printf("je lance :%d\n",getpid());
            struct fibonacci *test;
            test = (struct fibonacci*) InitSlave("localhost");
            lock_read(test,sizeof(test));
            unlock_read(test, sizeof(test));
            printf("pid:%d end\n",getpid());
        }
    }
}