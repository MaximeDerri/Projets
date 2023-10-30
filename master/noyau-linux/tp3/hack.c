#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <dlfcn.h>

size_t read(int fd, void *buf, size_t nbytes)
{
        static void *handle = NULL;
        static char prev_action;
        ssize_t ret = 0;
        ssize_t (*other)(int, void *, size_t);
        other = dlsym(RTLD_NEXT, "read");

        if(other == NULL) {
                perror("PANIC");
                exit(1);
        }

        if (handle == NULL) {
                handle = dlopen("./libfunc.so", RTLD_NOW | RTLD_LOCAL | RTLD_NOLOAD); //recupérer handle de base pour pouvoir écraser quand nécessaire
                prev_action = 'r'; //r -> libfunc.so
        }

        ret = other(fd, buf, nbytes);
        
        if (*((char *)buf) == 'r' && prev_action != 'r') {
                dlclose(handle);
                handle = dlopen("./libfunc.so", RTLD_NOW | RTLD_LOCAL);
                prev_action = 'r';
        }
        else if (*((char *)buf) == 'i' && prev_action != 'i') {
                dlclose(handle);
                handle = dlopen("./libhackfunc.so", RTLD_NOW | RTLD_LOCAL); /*LAZY: quand fct appelé | NOW: force le lien directement */
                prev_action = 'i';
        }
        

        return ret;
}

/*
size_t read(int fd, void *buf, size_t nbytes)
{
    ssize_t ret = 0;
    ssize_t (*other)(int, void *, size_t);
    other = dlsym(RTLD_NEXT, "read");

    if(other == NULL) {
        perror("PANIC");
        exit(1);
    }
    ret = other(fd, buf, nbytes);
    if(*((char *)buf) == 'r') {
        *((char *)buf) = 'i';
    }
    return ret;
}
*/

/*
size_t read(int fd, void *buf, size_t nbytes)
{
    printf("Tchao !!!\n");
    *((char *)buf) = (char)'e';
    return 0;
}
*/