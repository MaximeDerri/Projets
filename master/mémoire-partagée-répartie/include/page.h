#ifndef PAGE_H
#define PAGE_H

#include <pthread.h>

struct page {
    pthread_mutex_t mutex;
};

#endif