//
// Created by changrui on 21/05/23.
//

#ifndef PSAR3_PAGE_STRUCT_H
#define PSAR3_PAGE_STRUCT_H

#include "slave.h"
#include "master.h"
#include <stdio.h>

struct page_struct{
    int taille;
    char pages[];
};
#endif //PSAR3_PAGE_STRUCT_H