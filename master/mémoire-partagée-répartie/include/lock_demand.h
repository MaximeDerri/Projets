#ifndef GIT_LOCK_DEMAND_H
#define GIT_LOCK_DEMAND_H

#include "slave.h"
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)


//maitre -> esclave
#define ERROR "ERROR\0"
#define OKLOC "OKLOC\0"
#define OKUNL "OKUNL\0"
#define INVAL "INVAL\0"
#define OKINV "OKINV\0"
#define GETPG "GETPG\0"
#define RELEA "RELEA\0"

//esclave -> maitre
#define UNLOR "UNLOR\0"
#define UNLOW "UNLOW\0"
#define LOCKR "LOCKR\0"
#define LOCKW "LOCKW\0"
//esclave <-> maitre
#define PAGES "PAGES\0"
//message type sont de taille fixe
#define MESSLEN 6
/********** servi par l'esclave ****************/
int8_t sendTo(int32_t socket,char* type,void* offset,size_t size);
int8_t send_lock_or_unlock(void *offset, void *ptr, size_t size, struct dsm_slave *slv, char *type);//size en octet! 1 page=PAGE_SIZE
int8_t send_updat(int32_t socket,void* nbrPg,void* numPage);
int8_t send_getPa(int32_t socket,void* offset,char *page);
int8_t send_pages(int32_t socket,void* offset,void* page);

//master -> slave
int8_t send_inval(int32_t socket,void* offset,void* size);

#endif //GIT_LOCK_DEMAND_H
