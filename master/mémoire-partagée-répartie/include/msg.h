#ifndef MSG_H
#define MSG_H

#include <stdint.h>

/*
struct op_msg {
    char msg[6];
    uint32_t offset;
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
*/

struct size_msg {
    char msg[6];
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
    uint32_t page_size;

    // Size of de la size, si egal a 8 signifie que l'archi du maitre est en 64 bits sinon 32, refuse connection
    // si pas la meme archi en maitre et esclave
    uint8_t archi;
};

#endif