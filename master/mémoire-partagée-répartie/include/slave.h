#ifndef SLAVE_H
#define SLAVE_H

#include "connection.h"
#include "msg.h"

#define _GNU_SOURCE
#include <sys/mman.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <endian.h>
#include <stdint.h>
#include <poll.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/userfaultfd.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <ucontext.h>

#include "connection.h"
#include "request.h"

#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define PAGE_SHIFT log2l(PAGE_SIZE)

struct dsm_slave {
    void *ptr; //base du segment de mémoire partagée
    void *page;
    struct connection co;
    //struct dsm_slave *next; //dans le cas où un esclave est soumis à plusieurs maitre
    pthread_mutex_t mutex; //bloquer le thread (accès socket) ou bloquer dans lock (accès socket)
    pthread_t th;
    // Pour la size dans master et ici present, faudra utiliser des macros pour voir l'architecture pour changer de 32 a 64 en fonction
    
    //taille du segment | arrondir à la page supérieur
#ifndef ARCHI
    uint64_t size;
#endif
#ifdef ARCHI
#if ARCHI == 32
    uint32_t size;
#else
    uint64_t size;
#endif
#endif

    int32_t uf_fd;
    uint8_t run;
};

struct slave {
    struct slave* next;
    struct slave* prev;
    struct connection co;
    // 1 si connecte, 0 sinon
    // uint8_t is_used;
    uint8_t page_map[];
};

//initialise un esclave
//prend le nom de l'host en paramètre et son port
void* init_slave(char* host_name, uint16_t port);
int destroy_slave();
int8_t lock(void* adr, size_t size, char *type);
int alter_protect_page(int prot, void *addr, unsigned long size);

#endif
