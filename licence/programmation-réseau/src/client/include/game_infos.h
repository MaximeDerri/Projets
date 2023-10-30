#ifndef GAME_INFOS_H
#define GAME_INFOS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


#define LOCALHOST 0 //1 -> localhost, 0 -> lulu
#define DEFAULT_TCP_PORT "4501"

#define ID_LEN 8
#define CODE_LEN 5
#define TAIL_LEN 3
#define PORT_LEN 4
#define IP_LEN 15
#define MSG_LEN 200
#define COM_LEN 2
#define POS_LEN 3
#define PTS_LEN 4
#define TXT_CHANNEL_MAX_LEN 550 //to be large and having the possibility to personalize
                               //the message before sending to xterm by named pipe


//string const
extern const char *PATH_PIPE;
extern const char *RED;    //err from serv (ex: start but user is not in a party)
extern const char *BLUE;   //msg
extern const char *YELLOW; //infos serv (ex: ghost move, points)
extern const char *RESET;  //back in white


extern int running; //to indicate the thread and in_game() to stop


/*
txt_channel contain the named pippe fd for textual channel.
We will not use GUI, so we will print mail, private msg in this pipe.
The pipe will be open by an xterm instance to have a smooth textual channel,
to avoid scanf and printf on the same time (else the client wait a command
from the player to display msg after that...) and having a better view.

udp and multicast msg will be listened by one thread
*/
struct game_infos {
    struct sockaddr_in tcp_addr; //for tcp
    struct sockaddr_in udp_addr; //for udp
    struct sockaddr_in mc_addr;  //for multicast
    int tcp_socket;
    int udp_socket;
    int mc_socket;
    int txt_channel; //fd of the nammed pipe
    int is_register;
    char id[ID_LEN + 1];
};



//set the bases of game_infos struct. Return 0 on success and -1 on error
//udp_port, because we set it directly, and argv contain:
//id dans tcp_port for getaddrinfo
//prep pipe, tcp, udp
int set_infos(struct game_infos *infos, char *id, char *tcp_port, uint16_t udp_port);

//close FDs
void clear_infos(struct game_infos *infos);

#endif