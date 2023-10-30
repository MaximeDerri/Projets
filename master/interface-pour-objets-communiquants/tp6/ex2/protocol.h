#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <endian.h>
#include <signal.h>
#include <poll.h>

#define BYTE_LEN sizeof(uint32_t)
#define NAME_MAX_LEN 32
#define REQ_LEN ((BYTE_LEN * 2) + NAME_MAX_LEN)

#define VOTE 0x1
#define ACCEPT_VOTE 0x2
#define ERROR 0x3

#define ETE 0x1
#define HIVER 0x2

#define PENDING_CLI 10


#endif