#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "game_infos.h"
#include "command.h"
#include "txt_channel.h"
#include "io_tools.h"


//check id
//return 0 on success and -1 on error
int check_id(char *id);

//check port
//return 0 on success and -1 on error
int check_port(char *port_s, uint16_t *port);

//(join / leave party, etc...) to (START***).
//port -> udp_port
//return 0 on succes and -1 on error
int begin_game(struct game_infos *infos, char *port);

//launch the game
//launch a thread who lisen udp/multicast
//start xterm process (listening on the named pipe for text channel)
//return 0 on succes and -1 on error
int in_game(struct game_infos *infos);


#endif