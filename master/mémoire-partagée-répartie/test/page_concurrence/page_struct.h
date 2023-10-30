#include <unistd.h>
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)

#ifndef PSAR2_PAGE_STRUCT_H
#define PSAR2_PAGE_STRUCT_H
struct page_struct{
    int taille;
    char pages[];
};
#endif //PSAR2_PAGE_STRUCT_H