#include "client.h"


int check_port(char *port_s, uint16_t *port) {
    short tmp_port = 0;
    if(strlen(port_s) != PORT_LEN) {
        printf("Bad len for arg port\n");
        return -1;
    }

    tmp_port = (short)strtol(port_s, NULL, 10);
    // in the good scale and not sup than 4 char | no need to hade 0 in front
    if (tmp_port < 1024 || tmp_port > 9999) {
        printf("Bad arg for port\n");
        return -1;
    }
    *port = (uint16_t)tmp_port;

    return 0;
}

int check_id(char *id) {
    if(strlen(id) != ID_LEN) {
        printf("Bad id, it should be 8 characters long\n");
        return -1;
    }
    else
        return 0;
}


int begin_game(struct game_infos *infos, char *port) {
    int n, r = 0;
    char com[COM_LEN + 1];

    if (gl_aux_com(infos) < -1)
        return -1;

    printf("\n%sUse 'help' command to see all the other commands%s\n\n", YELLOW, RESET);
    //now, the user can input command and communicate with the server
    //the loop ended with 'sp' command (start party) -> terminate this function and return to the main
    //see command.h/.c for commands


    while (1) {
        printf("==========\n");
        memset(com, '\0', sizeof(char)*(COM_LEN+1));
        r = stdin_listen(com, &n, NULL, NULL); // in bad input, n == -1
        if (r == -2)                           // listen STDIN and extract datas
            return -1;
        if (r == -1) // bad input
            continue;

        //~switch on com (we can't use switch on char*, or we can use hashing methodes but collisons are possibles)
        if (strcmp(com, "jp") == 0) {
            if (n < 0) { // check args
                printf("%s%s%s\n", RED, BAD_COM, RESET);
            }
            else if (n > 255) {
                printf("%sThe number needs to be in between [0,255]%s\n", RED, RESET);
            }
            else {
                if (jp_com(infos, port, (uint8_t)n) < -1)
                    return -1;
            }

        }
        else if (strcmp(com, "np") == 0) {
            if (np_com(infos, port) < -1)
                return -1;

        }
        else if (strcmp(com, "up") == 0) {
            if (up_com(infos) < -1)
                return -1;

        }
        else if (strcmp(com, "gl") == 0) {
            if (gl_com(infos) < -1)
                return -1;

        }
        else if (strcmp(com, "pl") == 0) {
            if (n < 0) { // check args
                printf("%s%s%s\n", RED, BAD_COM, RESET);
            }
            else if (n > 255) {
                printf("%sThe number needs to be in between [0,255]%s\n", RED, RESET);
            }
            else {
                if (pl_com(infos, (uint8_t)n) < -1)
                    return -1;
            }

        }
        else if (strcmp(com, "sm") == 0) {
            if (n < 0) { // check args
                printf("%s%s%s\n", RED, BAD_COM, RESET);
            }
            else if (n > 255) {
                printf("%sThe number needs to be in between [0,255]%s\n", RED, RESET);
            }
            else {
                if (sm_com(infos, (uint8_t)n) < -1)
                    return -1;
            }

        }
        else if (strcmp(com, "sp") == 0) {
            r = sp_com(infos);
            if (r == -2)
                continue;
            else if(r == -1)
                return -1;
            else
                return 0; // the player is now ready ! we can go on the next step - only return witout error

        }
        else if(strcmp(com, "ws") == 0) {
            if (n < 0) { // check args
                printf("%s%s%s\n", RED, BAD_COM, RESET);
            }
            else if (n > 255) {
                printf("%sThe number needs to be in between [0,255]%s\n", RED, RESET);
            }
            else {
                if (ws_com(infos, (uint8_t)n) < -1)
                    return -1;
            }

        }
        else {
            printf("%s%s%s\n", RED, HELP, RESET);
        }
    }

    return -1; // should be never reach
}


int in_game(struct game_infos *infos) {
    int r, n = 0;
    char msg[MSG_LEN+1];
    char id[ID_LEN+1];
    char com[COM_LEN+1];
    pthread_t th;

    if(welcome(infos) < 0)
        return -1;

    //launch xterm
    r = fork();
    if(r < 0) { //err
        perror("Errr from fork()");
        return -1;
    }
    if(r == 0) {
        char buff[100] = "cat ";
        strcat(buff, PATH_PIPE);
        execlp("xterm", "xterm", "-e", buff, NULL);

        //err
        printf("Error from in_game() - execlp()\n");
        exit(EXIT_FAILURE);
    }

    //open txt_chanel
    if((infos->txt_channel = open(PATH_PIPE, O_WRONLY)) < 0) {
        perror("Error, open() failed to open txt_channel (named pipe)");
        return -1;
    }

    //thread
    r = pthread_create(&th, NULL, listen_udp_mc, infos);
    if(r != 0) {
        perror("Error pthread_create() for listen_udp_mc()");
        return -1;
    }
    printf("\n%sGame start !!!\nUse 'help' command to see all the other commands%s\n\n", YELLOW, RESET);

    //listen user input like begin_game TODO
    //on error, we chage running to 0 -> interrupt thread
    while(running) {
        printf("==========\n");
        memset(msg, '\0', sizeof(char) * (MSG_LEN+1));
        memset(id, '\0', sizeof(char) * (ID_LEN+1));
        memset(com, '\0', sizeof(char) * (COM_LEN+1));
        r = stdin_listen(com, &n, id, msg); // in bad input, n == -1
                                            // listen STDIN and extract datas
        if(r == 1) { //the game just reach the end
            break;
        }
        if (r == -2) {
            running = 0;
            return -1;
        }                           
        if (r == -1) { // bad input
            printf("%s%s%s\n", RED, HELP, RESET);
            continue;
        }
        
        if(strcmp(com,"mu") == 0) {
            if (n < 0) { // check args
                printf("%s%s%s\n", RED, BAD_COM, RESET);
            }
            else if (n > 999) {
                printf("%sThe number needs to be in between [0,999]%s\n", RED, RESET);
            }
            else {
                if (mv_com(infos, 'u', n) < -1) {
                    running = 0;
                    return -1;
                }
            }

        }
        else if(strcmp(com,"md") == 0) {
            if (n < 0) { // check args
                printf("%s%s%s\n", RED, BAD_COM, RESET);
            }
            else if (n > 999) {
                printf("%sThe number needs to be in between [0,999]%s\n", RED, RESET);
            }
            else {
                if (mv_com(infos, 'd', n) < -1) {
                    running = 0;
                    return -1;
                }
            }

        }
        else if(strcmp(com,"mr") == 0) {
            if (n < 0) { // check args
                printf("%s%s%s\n", RED, BAD_COM, RESET);
            }
            else if (n > 999) {
                printf("%sThe number needs to be in between [0,999]%s\n", RED, RESET);
            }
            else {
                if (mv_com(infos, 'r', n) < -1) {
                    running = 0;
                    return -1;
                }
            }

        }
        else if(strcmp(com,"ml") == 0) {
            if (n < 0) { // check args
                printf("%s%s%s\n", RED, BAD_COM, RESET);
            }
            else if (n > 999) {
                printf("%sThe number needs to be in between [0,999]%s\n", RED, RESET);
            }
            else {
                if (mv_com(infos, 'l', n) < -1) {
                    running = 0;
                    return -1;
                }
            }

        }
        else if(strcmp(com, "pl") == 0) {
            if(pl_game_com(infos) < -1) {
                running = 0;
                return -1;
            }

        }
        else if(strcmp(com, "am") == 0) {
            if(strlen(msg) == 0)
                printf("%s%s%s\n", RED, BAD_COM, RESET);
            else {
                if(am_com(infos, msg) < -1) {
                    running = 0;
                    return -1;
                }
            }

        }
        else if(strcmp(com, "pm") == 0) {
            if(strlen(msg) == 0 || strlen(id) != ID_LEN)
                printf("%s%s%s\n", RED, BAD_COM, RESET);
            else {
                if(pm_com(infos, msg, id) < -1) {
                    running = 0;
                    return -1;
                }
            }


        }
        else if(strcmp(com, "lg") == 0) {
            if(lg_com(infos) < -1) {
                running = 0;
                return -1;
            }
            
        }
        else if(strcmp(com, "ub") == 0) {
            if(ub_com(infos) < -1) {
                running = 0;
                return -1;
            }
            
        }
        else if(strcmp(com, "ur") == 0) {
            if(ur_com(infos) < -1) {
                running = 0;
                return -1;
            }
            
        }
        else {
            printf("%s%s%s\n", RED, HELP, RESET);
        }
    }
    
    pthread_join(th, NULL);
    return 0;
}





// MAIN
int main(int argc, char *argv[]) {
    int ret_code = EXIT_FAILURE;
    struct game_infos infos;
    uint16_t tcp_port, udp_port = 0;
    char tcp_port_s[PORT_LEN+1];
    char udp_port_s[PORT_LEN+1];
    tcp_port_s[PORT_LEN] = '\0';
    udp_port_s[PORT_LEN] = '\0';
    
    errno = 0;

    if(argc == 4) { // ./client id tcp_port udp_port
        if(check_id(argv[1]) < 0)
            return ret_code;
        
        if(check_port(argv[2], &tcp_port) < 0)
            return ret_code;

        if(check_port(argv[3], &udp_port) < 0)
            return ret_code;
        
        memcpy(tcp_port_s, argv[2], sizeof(char) * PORT_LEN);
        memcpy(udp_port_s, argv[3], sizeof(char) * PORT_LEN);
    }
    else if(argc == 3) { // ./client id udp_port
        if(check_id(argv[1]) < 0)
            return ret_code;
        
        if(check_port(argv[2], &udp_port) < 0)
            return ret_code;
        
        memcpy(tcp_port_s, DEFAULT_TCP_PORT, sizeof(char) * PORT_LEN);
        memcpy(udp_port_s, argv[2], sizeof(char) * PORT_LEN);
    }
    else { //err
        printf("Bad args:\n* ./client id tcp_port udp_port\n* ./client id udp_port\n");
        return ret_code;
    }


    // infos prep
    if (set_infos(&infos, argv[1],tcp_port_s, udp_port) < 0)
        return ret_code; // err is already print

    // connection to serv
    if (connect(infos.tcp_socket, (struct sockaddr *)&infos.tcp_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Error from connect()");
        return ret_code;
    }

    // begin
    if (begin_game(&infos, udp_port_s) < 0) {
        clear_infos(&infos);
        return ret_code; // err is already print
    }

    //game
    if(in_game(&infos) == 0) {
        ret_code = EXIT_SUCCESS;
    }
    
    //in case of success or failure, we need to close all FDs
    clear_infos(&infos);

    return EXIT_SUCCESS;
}
