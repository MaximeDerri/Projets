#ifndef TXT_CHANNEL
#define TXT_CHANNEL

#include <sys/select.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>

#include "game_infos.h"
#include "command.h"



//function - thread for msg
void *listen_udp_mc(void *ptr);

//write into channel the msg -> xterm
//return 0 on succes and -1 on error
int write_txt_channel(int channel, char *buff, int length);

//prep / parse msg (am/pm) and write them into channel
//return 0 on succes and -2 on error (function will be call by check_mc/udp_sock())
int prep_msg_and_send(int channel, char *buff, int len, char *id, char *dest);

//check multicast req
//retur 0 on succes, -1 on protocol error and -2 on error
int check_mc_sock(struct game_infos *infos);

//check udp req
//retur 0 on succes, -1 on protocol error and -2 on error
int check_udp_sock(struct game_infos *infos);

#endif