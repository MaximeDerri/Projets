#ifndef COMMAND_H
#define COMMAND_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <endian.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "game_infos.h"


//string const
extern const char *HELP;
extern const char *BAD_COM;



//verify the header / tail of the protocol
//return 0 on succes and -1 on protocol error
int verif_protocol(char *proto, char *target); //targer -> thing to compare (it come from req)

//verif CODE and TAIL
//return 0 on success and -1 on protocol error
int verif_code_tail(char *buff, int len, char *code, char *tail);


//join party
//return 0 on succes, -1 on protocol error and -2 on error (ex: recv/send)
int jp_com(struct game_infos *infos, char *port, uint8_t m);

//new party
//return 0 on succes, -1 on protocol error and -2 on error (ex: recv/send)
int np_com(struct game_infos *infos, char *port);

//unreg from the party
//return 0 on succes, -1 on protocol error and -2 on error (ex: recv/send)
int up_com(struct game_infos *infos);

//game list
//return 0 on success and -2 on error
int gl_com(struct game_infos *infos);

//aux for gl_com - this part don't send the request (use after connect())
//print parties (after GAMES, GAME?) rec from tcp socket
//return 0 on succes, -1 on protocol error and -2 on error (ex: recv/send)
int gl_aux_com(struct game_infos *infos);

//aux for gl_aux_com()
//return 0 on succes, -1 on protocol error and -2 on error (ex: recv/send)
int print_parties(struct game_infos *infos, uint8_t b);

//player list of party m
//return 0 on succes, -1 on protocol error and -2 on error (ex: recv/send)
int pl_com(struct game_infos *infos, uint8_t m);

//aux for pl_com()
//print players
//return 0 on succes, -1 on protocol error and -2 on error (ex: recv/send)
int print_players(struct game_infos *infos, uint8_t m);

//maze's size
//return 0 on succes, -1 on protocol error and -2 on error (ex: recv/send)
int sm_com(struct game_infos *infos, uint8_t m);

//start party - ready
//return 0 on succes and -1 on error (ex: recv/send) or -2 if we try to start but we are not in a party
//NB: no -1 because we don't receive reply from the serv AND to have the same idea on errcode returned by the others functions
int sp_com(struct game_infos *infos);

//tell who is ready or not on party m
//return 0 on success, -1 on protocol error and -2 on error
int ws_com(struct game_infos *infos, uint8_t m);

//aux for ws_com - list ready or not
//return 0 on success, -1 on protocol error and -2 on error
int print_ready(struct game_infos *infos, uint8_t m);


/* ***** */


//receive of WELCO
//bind udp / multicast
//return 0 on success, -1 on protocol error and -2 on error
int welcome(struct game_infos *infos);

//initial position
//return 0 on success, -1 on protocol error and -2 on error
int init_pos(struct game_infos *infos);

//check GOBYE
//return 0 on success and -1 on protocol error
int gobye(char *buff);

//move up/down/right left deppending of d: ('u', 'd', 'r', 'l') by b blocks
//if b == 0 we send too
//return 0 on succes, -1 on protocol error and -2 on error
int mv_com(struct game_infos *infos, char d, int b);

//list player from the actual game
//return 0 on succes, -1 on protocol error and -2 on error
int pl_game_com(struct game_infos *infos);

//aux for pl_game_com()
//print
//return 0 on succes, -1 on protocol error and -2 on error
int print_players_game(struct game_infos *infos);

//send a global message < 200 char
//'all' msg
//return 0 on succes, -1 on protocol error and -2 on error
int am_com(struct game_infos *infos, char *msg); 

//send a private message to id, < 200 char and length(id) = ID_LEN
//return 0 on succes, -1 on protocol error and -2 on error
int pm_com(struct game_infos *infos, char *msg, char *id);

//leave game
//return 0 on success, -1 on protocol error and -2 on error
int lg_com(struct game_infos *infos);

//use bomb
//return 0 on success, -1 on protocol error and -2 on error
int ub_com(struct game_infos *infos);

//use radar
//return 0 on success, -1 on protocol error and -2 on error
int ur_com(struct game_infos *infos);

//aux for ur_com
//return 0 on success, -1 on protocol error and -2 on error
int print_radar(struct game_infos *infos);


#endif