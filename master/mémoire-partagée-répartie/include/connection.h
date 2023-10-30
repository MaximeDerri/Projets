#ifndef CONNECTION_H
#define CONNECTION_H

#include <sys/socket.h>
#include <netinet/in.h>

// Definir un port d'ecoute global
#define PORTSERV 4072

struct connection {
    struct sockaddr_in adr;
    int32_t socket;
};

#endif