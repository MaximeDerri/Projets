#ifndef IO_TOOLS_H
#define IO_TOOLS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "game_infos.h"



//clear stdin
void drain_stdin();

//aux for in_game() and begin_game()
//listen STDIN and extract datas
//length(com) = 3 -> 2 char command and 1 \0
//n -> nbr or -1 if any nbr was not find
//length(id) = 9 (\0) (we can look the length to see if id was set by in_listen())
//length(msg)  = 201 (\0), like id to see any pb
//args from STDIN are n  or  id mess
//if (from args) id or msg == NULL : they are skip (begin_game())
//return 0 on succes and -1 on bad input and -2 on error
//return 1 if running = 0 -- prevent printing error if the game has ended
int stdin_listen(char *com, int *n, char *id, char *msg);


#endif