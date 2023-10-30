#ifndef MASTER_H
#define MASTER_H

// Pour regler mes problemes avec le flag ANONYMOUS pour map
#define _GNU_SOURCE
// Pour sigaction
#define _XOPEN_SOURCE 700
// Decrire une macro pour faire varier en fonction des tailles des pages de la machine
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)

#include "connection.h"
#include "slave.h"
#include "page.h"
#include "msg.h"

#include <sys/mman.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <endian.h>

// Decrire une macro pour faire varier en fonction des tailles des pages de la machine
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
// Nb max de slave pour 1 maitre
#define MAX_SLAVE 20

// Pour les operations de LOCK et UNLOCK pour la gestion des esclaves
#define UNLOCK 0
#define LOCK 1

struct dsm_master {
    void *ptr; //base du segment de mémoire partagée
    pthread_mutex_t mutex;
    struct page *pages;
    struct slave *slaves;
    struct connection co;
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
};

// Sructure conteneur contenant les differentes infos pour les fonctions lock et unlock
struct lock_struct {
    void *addr;
    void *_offset;
    void *_size;
    int32_t fd;
    uint8_t operation; 
};

void *init_master(size_t size);
void loop_master();
void *listen_t(void *arg);
int destroy_master();

#endif