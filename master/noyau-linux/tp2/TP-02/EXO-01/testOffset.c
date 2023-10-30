#include <stdio.h>

#include "commit.h"

int main(int argc, char *argv[]) {
    struct commit com;
    void *base = &com;
    void *vers = &com.version;
    
    printf("%p\n",&com);
    printf("%p\n", commitOf(&com.version));
    //&((struct commit *) 0)->id)
}