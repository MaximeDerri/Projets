#include "command.h"


//string const
const char *HELP = "List of commands:\n\
before launch:\n\
\tjp n -> join the party n\n\
\tnp   -> create a new party\n\
\tup   -> unregister from the current party\n\
\tgl   -> list all the parties\n\
\tpl n -> list players from the party n\n\
\tsm n -> size of maze in party n\n\
\tsp   -> inform the server that you are ready to start\n\
\tws n -> tell who is ready or not in the party \n\
\n\nduring game:\n\
\tmu | md | mr | ml n -> move up | down | right | left by n blocks\n\
\tpl   -> list players from the current party\n\
\tam   -> send a global message\n\
\tpm id m -> send to the message m to the id\n\
\tlg   -> leave the game\n\
\tub   -> use the bomb ability\n\
\tur   -> use the radar ability";
const char *BAD_COM = "Unknown command or bad args - look 'help' command";


int verif_protocol(char *proto, char *target) {
    if(strcmp(proto, target) == 0) {
        return 0;
    }
    else {
        printf("Error, the protocol was not respected - expected '%s'\n", proto);
        return -1;
    }
}


int verif_code_tail(char *buff, int len, char *code, char *tail) {
    char req[CODE_LEN+1];

    memcpy(req, buff+(len-TAIL_LEN), sizeof(char) * TAIL_LEN);
    req[TAIL_LEN] = '\0';
    if(verif_protocol(tail, req) != 0)
        return -1;

    memcpy(req, buff, sizeof(char) * CODE_LEN);
    req[CODE_LEN] = '\0';
    if(verif_protocol(code, req) != 0)
        return -1;

    return 0;
}


int jp_com(struct game_infos *infos, char *port, uint8_t m) { //join party
    int len_buff = 24; //lenght max
    int len_tmp = CODE_LEN;
    int r = 0;
    char buff[len_buff]; //recv / send
    char *header = "REGIS";
    char *tail = "***";

    if(strlen(port) != 4) {
        printf("char *port arg needs to be 4 chars long\n");
        return -2;
    }

    //req
    memcpy(buff, header, sizeof(char) * len_tmp);
    buff[len_tmp] = ' ';
    memcpy(buff+(len_tmp+1), infos->id, sizeof(char) * ID_LEN);
    len_tmp += 1 + ID_LEN;
    buff[len_tmp] = ' ';
    memcpy(buff+(len_tmp+1), port, sizeof(char) * PORT_LEN);
    len_tmp += 5;
    buff[len_tmp] = ' ';
    buff[len_tmp+ 1] = m; //party id
    len_tmp += 2;
    memcpy(buff+len_tmp, tail, sizeof(char) * TAIL_LEN);

    r = send(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
    if(r < 0) {
        perror("Error from send() in jp_com()");
        return -2;
    }

    //reply
    len_tmp = 10;
    r = recv(infos->tcp_socket, buff, sizeof(char) * len_tmp, 0);
    if(r < 0) {
        perror("Error from recv() in jp_com()");
        return -2;
    }

    //verif protocol
    if(r == len_tmp) {
        if(verif_code_tail(buff, r, "REGOK", tail) < 0) {
            printf("Protocol error from jp_com()\n");
            return -1;
        }
        
        if(buff[6] != m) {
            printf("Error, the server returned an incorrect party number\n");
            return -2;
        }

        infos->is_register = 1;
        printf("%sYou successfully joined the party(%d) !%s\n", YELLOW, (int)buff[6], RESET);
        return 0;

    }
    else if(r == (CODE_LEN + TAIL_LEN)) {
        buff[CODE_LEN + TAIL_LEN] = '\0';
        if(verif_protocol("REGNO***", buff) != 0)
            return -1;
        
        printf("%sYou haven't joined the party...%s\n", RED, RESET);
        return 0;

    }
    else {
        printf("Protocol error from jp_com()\n");
        return -1;
    }
}


int np_com(struct game_infos *infos, char *port) { //new party
    int len_buff = 22; //lenght max
    int len_tmp = CODE_LEN;
    int r = 0;
    char buff[len_buff]; //recv / send
    char *header = "NEWPL";
    char *tail = "***";

    if(strlen(port) != 4) {
        printf("char *port arg needs to be of 4 chars long\n");
        return -2;
    }
    
    //req
    memcpy(buff, header, sizeof(char) * len_tmp);
    buff[len_tmp] = ' ';
    memcpy(buff+(len_tmp+1), infos->id, sizeof(char) * ID_LEN);
    len_tmp += 1 + ID_LEN;
    buff[len_tmp] = ' ';
    memcpy(buff+(len_tmp+1), port, sizeof(char) * PORT_LEN);
    len_tmp += 5;
    memcpy(buff+len_tmp, tail, sizeof(char) * TAIL_LEN);

    r = send(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
    if(r < 0) {
        perror("Error from send() in np_com()");
        return -2;
    }

    //reply
    len_tmp = 10;
    r = recv(infos->tcp_socket, buff, sizeof(char) * len_tmp, 0);
    if(r < 0) {
        perror("Error from recv() in np_com()");
        return -2;
    }

    //verif protocol
    if(r == len_tmp) {
        if(verif_code_tail(buff, r, "REGOK", tail) < 0) {
            printf("Protocol error from np_com()\n");
            return -1;
        }

        infos->is_register = 1;
        printf("%sThe party(%d) was created successfully !%s\n", YELLOW, (int)buff[6], RESET);
        return 0;

    }
    else if(r == (CODE_LEN + TAIL_LEN)) {
        buff[CODE_LEN + TAIL_LEN] = '\0';
        if(verif_protocol("REGNO***", buff) != 0)
            return -1;
        
        printf("%sThe creation of the party has been refused...%s\n", RED, RESET);
        return 0;

    }
    else {
        printf("Protocol error from np_com()\n");
        return -1;
    }
}


int up_com(struct game_infos *infos) { //unreg party
    int len_buff = 10; //lenght max
    int r = 0;
    char buff[len_buff];
    char *tmp_s = "UNREG***";
    
    //req
    memcpy(buff, tmp_s, sizeof(char) * (CODE_LEN + TAIL_LEN));
    if(send(infos->tcp_socket, buff, sizeof(char) * strlen(tmp_s), 0) < 0) {
        perror("Error from send() in up_com()");
        return -2;
    }

    //reply
    r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
    if(r < 0) {
        perror("Error from recv() in up_com()");
        return -2;
    }

    //verif protocol
    if(r == len_buff) {
        if(verif_code_tail(buff, r, "UNROK", "***") < 0) {
            printf("Protocol error from up_com()\n");
            return -1;
        }

        infos->is_register = 0;
        printf("%sYou have left the party(%d) successfully%s\n", YELLOW, (int)buff[6], RESET);
        return 0;

    }
    else if(r == (CODE_LEN + TAIL_LEN)) { // < len_buff
        buff[CODE_LEN + TAIL_LEN] = '\0';
        if(verif_protocol("DUNNO***", buff) != 0)
            return -1;
        
        printf("%sYou haven't left any party... Are you sure you have joined one?%s\n", RED, RESET);
        return 0;

    }
    else {
        printf("Protocol error from up_com()\n");
        return -1;
    }
}


int gl_com(struct game_infos *infos) {
    char buff[CODE_LEN + TAIL_LEN + 1] = "GAME?***";

    //req
    if(send(infos->tcp_socket, buff, sizeof(char) * (CODE_LEN + TAIL_LEN), 0) < 0) {
        perror("Error from send() in gl_com()");
        return -2;
    }
    
    return gl_aux_com(infos);
}


int gl_aux_com(struct game_infos *infos) {
    int len_buff = 10; //lenght max
    int r = 0;
    char buff[len_buff];

    //recv game list
    r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
    if(r < 0) {
        perror("Error from recv() in gl_aux_com()");
        return -2;
    }

    //verif protocol
    if(r != len_buff) {
        printf("Protocol error from gl_aux_com()\n");
        return -1;
    }

    if(verif_code_tail(buff, r, "GAMES", "***") < 0) {
            printf("Protocol error from gl_aux_com()\n");
            return -1;
        }
    
    //print parties
    return print_parties(infos, (uint8_t)buff[6]);
}


int print_parties(struct game_infos *infos, uint8_t b) { //game list
    int len_buff = 12;
    int r = 0;
    char buff[len_buff+1];

    if(b == 0) {
        printf("%sNo parties are waiting players%s\n", YELLOW, RESET);
        return 0;
    }

    printf("%sParties:%s\n",YELLOW, RESET);
    for(uint8_t i = 0; i < b; i++) {
        r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
        if(r < 0) {
            perror("Error recv() from print_parties()");
            return -2;
        }

        //verif protocol
        if(r != len_buff) {
            printf("Protocol error from print_parties()\n");
            return -1;
        }
        if(verif_code_tail(buff, r, "OGAME", "***") < 0) {
            printf("Protocol error from print_parties()\n");
            return -1;
        }

        printf("\t%sid party: %d  |  players: %d%s\n", YELLOW, (int)buff[6], (int)buff[8], RESET);
    }

    return 0;
}


int pl_com(struct game_infos *infos, uint8_t m) {
    int len_buff = 10; //lenght max
    int len_tmp = CODE_LEN;
    char buff[len_buff];
    char *header = "LIST?";
    char *tail = "***";

    //req
    memcpy(buff, header, sizeof(char) * len_tmp);
    buff[len_tmp] = ' ';
    buff[len_tmp+1] = m;
    len_tmp+=2;
    memcpy(buff+len_tmp, tail, sizeof(char) * TAIL_LEN);

    if(send(infos->tcp_socket, buff, sizeof(char) * len_buff, 0) < 0) {
        perror("Error from send() in pl_com()");
        return -2;
    }

    return print_players(infos, m);
}


int print_players(struct game_infos *infos, uint8_t m) {
    uint8_t players = 0;
    int len_buff = 17;
    int r = 0;
    char buff[len_buff];

    //verif protocol (provoked by pl_com())
    int len_tmp = 12; //len of first reply 
    r = recv(infos->tcp_socket, buff, sizeof(char) * len_tmp, 0);
    if(r < 0) {
        perror("Error from recv() in print_players()");
        return -2;
    }
    
    if(r == len_tmp) {
        if(verif_code_tail(buff, r, "LIST!", "***") < 0) {
            printf("Protocol error from print_players()\n");
            return -1;
        }

        //getting datas
        if(buff[6] != m) {
            printf("Error, the server returned an incorrect party number\n");
            return -2;
        }
        players = buff[8];

    }
    else if(r == (CODE_LEN+TAIL_LEN)) { // < len_tmp
        buff[CODE_LEN + TAIL_LEN] = '\0';
        if(verif_protocol("DUNNO***", buff) != 0)
            return -1;
        
        printf("%sThe party(%d) does not exist%s\n", YELLOW, (int)m, RESET);
        return 0;
        
    }
    else {
        printf("Protocol error from print_players()\n");
        return -1;
    }

    //print
    printf("%sParty(%d) has %d player(s) :%s\n", YELLOW, (int)m, players, RESET);
    for(uint8_t i = 0; i < players; i++) {
        r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
        if(r < 0) {
            perror("Error recv() from print_players()");
            return -2;
        }

        //verif protocol
        if(r != len_buff) {
            printf("Protocol error from print_players()\n");
            return -1;
        }
        if(verif_code_tail(buff, r, "PLAYR", "***") < 0) {
            printf("Protocol error from print_players()\n");
            return -1;
        }

        buff[CODE_LEN + 1 + ID_LEN] = '\0';
        printf("\t%s%s%s\n", YELLOW, buff+(CODE_LEN + 1), RESET);
    }

    return 0;
}


int sm_com(struct game_infos *infos, uint8_t m) {
    int len_buff = 16; //lenght max
    int len_tmp = CODE_LEN;
    int r = 0;
    uint16_t heigh, width = 0;
    char buff[len_buff]; //recv / send
    char *header = "SIZE?";
    char *tail = "***";

    //req
    memcpy(buff, header, sizeof(char) * len_tmp);
    buff[len_tmp] = ' ';
    buff[len_tmp+1] = m;
    len_tmp += 2;
    memcpy(buff+len_tmp, tail, sizeof(char) * TAIL_LEN);
    len_tmp += 3;

    if(send(infos->tcp_socket, buff, sizeof(char) * len_tmp, 0) < 0) {
        perror("Error from send() in sm_com()");
        return -2;
    }

    //reply
    r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
    if(r < 0) {
        perror("Error from recv() in sm_com()");
        return -2;
    }

    //verif protcol
    if(r == len_buff) {
        if(verif_code_tail(buff, r, "SIZE!", tail) < 0) {
            printf("Protocol error from sm_com()\n");
            return -1;
        }

        if(buff[6] != m) {
            printf("Error, the server returned an incorrect party number\n");
            return -2;
        }
        
        //heigh is encoded on buff[8] and buff[9]
        heigh = buff[8] << 8 | buff[9];
        //width is encoded on buff[11] and buff[12]
        width = buff[11] << 8 | buff[12];
        //the 2 are received in little-endian
        heigh = le16toh(heigh);
        width = le16toh(width);

        //they need to be < 1000
        if(heigh > 1000 || width > 1000) {
            printf("Error, the maze size is too big\n");
            return -2;
        }

        printf("%s\n Maze's size (HEIGHT * WIDTH) = (%d * %d)%s\n", YELLOW, (int)heigh, (int)width, RESET);
        return 0;

    }
    else if(r == (CODE_LEN + TAIL_LEN)) {  // < len_buff
        buff[CODE_LEN + TAIL_LEN] = '\0';
        if(verif_protocol("DUNNO***", buff) != 0)
            return -1;
        
        printf("%sThe party(%d) does not exist%s\n", YELLOW, (int)m, RESET);
        return 0;

    }
    else {
        printf("Protocol error from sm_com()\n");
        return -1;
    }
}


int sp_com(struct game_infos *infos) {
    if(infos->is_register) {
        char tmp_s[CODE_LEN + TAIL_LEN + 1] = "START***";
        if(send(infos->tcp_socket, tmp_s, sizeof(char) * (CODE_LEN + TAIL_LEN), 0) < 0) {
            perror("Error from send() in sp_com()");
            return -1;
        }
        printf("%sYou are now ready! Waiting for START from other players...%s\n", YELLOW, RESET);
        return 0;
    }
    else {
        printf("%sYou can't ready up because you are not in a party%s\n", YELLOW, RESET);
        return -2;
    }
}


int ws_com(struct game_infos *infos, uint8_t m) {
    int len_buff = 10; //lenght max
    int len_tmp = CODE_LEN;
    char buff[len_buff];
    char *header = "STAR?";
    char *tail = "***";

    //req
    memcpy(buff, header, sizeof(char) * len_tmp);
    buff[len_tmp] = ' ';
    buff[len_tmp+1] = m;
    len_tmp+=2;
    memcpy(buff+len_tmp, tail, sizeof(char) * TAIL_LEN);

    if(send(infos->tcp_socket, buff, sizeof(char) * len_buff, 0) < 0) {
        perror("Error from send() in ws_com()");
        return -2;
    }

    return print_ready(infos, m);
}


int print_ready(struct game_infos *infos, uint8_t m) {
    uint8_t nb, test = 0;
    int len_buff = 19;
    int r = 0;
    char buff[len_buff];

    //verif protocol
    int len_tmp = 12; //len of first reply 
    r = recv(infos->tcp_socket, buff, sizeof(char) * len_tmp, 0);
    if(r < 0) {
        perror("Error from recv() in print_ready()");
        return -2;
    }
    
    if(r == len_tmp) {
        if(verif_code_tail(buff, r, "STAR!", "***") < 0) {
            printf("Protocol error from print_ready()\n");
            return -1;
        }

        //getting datas
        if(buff[6] != m) {
            printf("Error, the server returned an incorrect party number\n");
            return -2;
        }
        nb = buff[8];

    }
    else if(r == (CODE_LEN+TAIL_LEN)) { // < len_tmp
        buff[CODE_LEN + TAIL_LEN] = '\0';
        if(verif_protocol("DUNNO***", buff) != 0)
            return -1;
        
        printf("%sThe party(%d) does not exist%s\n", YELLOW, (int)m, RESET);
        return 0;
        
    }
    else {
        printf("Protocol error from print_ready()\n");
        return -1;
    }

    //print
    printf("%sParty(%d) - who is ready ?:%s\n", YELLOW, (int)m, RESET);
    for(uint8_t i = 0; i < nb; i++) {
        r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
        if(r < 0) {
            perror("Error recv() from print_ready()");
            return -2;
        }

        //verif protocol
        if(r != len_buff) {
            printf("Protocol error from print_ready()\n");
            return -1;
        }

        if(verif_code_tail(buff, r, "STARP", "***") < 0) {
            printf("Protocol error from print_ready()\n");
            return -1;
        }

        test = buff[15];
        buff[CODE_LEN + 1 + ID_LEN] = '\0';
        if(test)
            printf("\t%s%s START%s\n", YELLOW, buff+(CODE_LEN + 1), RESET);
        else
            printf("\t%s%s NOT START%s\n", YELLOW, buff+(CODE_LEN + 1), RESET);
    }

    return 0;
}



/* ***** */



int welcome(struct game_infos *infos) {
    int len_buff = 39; //lenght max
    int r = 0;
    uint8_t party, ghost = 0;
    uint16_t heigh, width = 0;
    char buff[len_buff];
    char ip[IP_LEN + 1];
    char port[PORT_LEN + 1];

    //reply
    r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
    if(r < 0) {
        perror("Error from recv() in welcome()");
        return -2;
    }

    //verif protcol
    if(r != len_buff) {
        printf("Protocol error from welcome()\n");
        return -1;
    }

    if(verif_code_tail(buff, r, "WELCO", "***") < 0) {
        printf("Protocol error from welcome()\n");
        return -1;
    }
    
    //getting datas
    party = (uint8_t)buff[6];

    //heigh is encoded on buff[8] and buff[9]
    heigh = buff[8] << 8 | buff[9];
    //width is encoded on buff[11] and buff[12]
    width = buff[11] << 8 | buff[12];
    //the 2 are received in little-endian
    heigh = le16toh(heigh);
    width = le16toh(width);

    //they need to be < 1000
    if(heigh > 1000 || width > 1000) {
        printf("Error, the maze size is too big\n");
        return -2;
    }

    ghost = (uint8_t)buff[14];

    //ip
    memcpy(ip, buff+16, sizeof(char) * IP_LEN);
    //verif ip pos + \0
    ip[IP_LEN] = '\0';
    for(int i = 0; ip[i] != '\0'; i++) {
        if(ip[i] == '#') {
            ip[i] = '\0';
            break;
        }
    } 

    port[PORT_LEN] = '\0';
    memcpy(port, buff+32, sizeof(char) * PORT_LEN);

    //udp
    if(bind(infos->udp_socket, (struct sockaddr *)&infos->udp_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Error from bind() udp_socket");
        return -2;
    }

    //multicast
    infos->mc_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if(infos->mc_socket== -1) {
        perror("Error from socket() for mc_socket");
        return -2;
    }
    
    int ok = 1;
    if(LOCALHOST == 1) 
        r = setsockopt(infos->mc_socket, SOL_SOCKET, SO_REUSEPORT, &ok, sizeof(ok));
    else
        r = setsockopt(infos->mc_socket, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok));
    
    if(r < 0) {
        perror("Error from setsockopt()");
        return -2;
    }

    short tmp_port = (short)strtol(port, NULL, 10);
    infos->mc_addr.sin_family = AF_INET;
    infos->mc_addr.sin_port= htons(tmp_port);
    infos->mc_addr.sin_addr.s_addr =  htonl(INADDR_ANY);
    if(bind(infos->mc_socket, (struct sockaddr *)&infos->mc_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Error from bind() mc_socket");
        return -2;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if(setsockopt(infos->mc_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Error from setsockopt()");
        return -2;
    }

    //printing infos
    printf("%sThe party(%d) has started. The maze have size (HEIGHT * WIDTH) = (%d * %d) and there are %d ghosts%s\n\n", YELLOW, (int)party, (int)heigh, (int)width, (int)ghost, RESET);

    return init_pos(infos);
}


int init_pos(struct game_infos *infos) {
    int len_buff = 25;
    int r = 0;
    char buff[len_buff];
    char x[POS_LEN+1];
    char y[POS_LEN+1];
    char id[ID_LEN+1];
    x[POS_LEN] = '\0';
    y[POS_LEN] = '\0';
    id[ID_LEN] = '\0';

    r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
    if(r < 0) {
        perror("Error from recv() in init_pos()");
        return -2;
    }
    
    //verif protcol
    if(r != len_buff) {
        printf("Protocol error from init_pos()\n");
        return -1;
    }

    if(verif_code_tail(buff, r, "POSIT", "***") < 0) {
        printf("Protocol error from init_pos()\n");
        return -1;
    }

    memcpy(id, buff+(CODE_LEN+1), sizeof(char) * ID_LEN);
    if(strcmp(id, infos->id) != 0) {
        printf("Error, server returned an incorrect id\n");
        return 2;
    }

    memcpy(x, buff+(CODE_LEN+ID_LEN+2), sizeof(char) * POS_LEN);
    memcpy(y, buff+(CODE_LEN+ID_LEN+POS_LEN+3),sizeof(char) * POS_LEN);
    printf("%sGo %s !!! Initial Position: (x,y) = (%s,%s)%s\n", YELLOW, id, x, y, RESET);

    return 0;
}


int gobye(char *buff) {
    buff[CODE_LEN + TAIL_LEN] = '\0';
    if(verif_protocol("GOBYE***", buff) != 0)
        return -1;

    running = 0; //stop
    printf("%sGame has ended\n%s", YELLOW, RESET);
    return 0;
}


int mv_com(struct game_infos *infos, char d, int b) {
    int len_buff = 21;
    int len_tmp = CODE_LEN;
    int r = 0;
    char buff[len_buff];
    char x[POS_LEN+1];
    char y[POS_LEN+1];
    char pts[PTS_LEN+1];
    char num[3];
    char num_tmp[4];
    char *tail = "***";
    x[POS_LEN] = '\0';
    y[POS_LEN] = '\0';
    pts[PTS_LEN] = '\0';
    memcpy(num, "000", sizeof(char) * 3);

    if(b < 0 || b > 999) {
        printf("%sBad number from mv_com()%s\n", RED, RESET);
        return -1;
    }
    r = sprintf(num_tmp, "%d", b);
    if(r < 0)  {
        perror("Error sprintf() in mv_com");
        return -2;
    }

    //req
    if(d == 'u')
        memcpy(buff, "UPMOV",  sizeof(char) * CODE_LEN);
    else if(d == 'd')
         memcpy(buff, "DOMOV",  sizeof(char) * CODE_LEN);
    else if(d == 'l')
         memcpy(buff, "LEMOV",  sizeof(char) * CODE_LEN);
    else if(d == 'r')
         memcpy(buff, "RIMOV",  sizeof(char) * CODE_LEN);
    else {
        printf("%sUnknown direction from mv_com()%s\n", RED, RESET);
        return -1;
    }
    buff[len_tmp] = ' ';
    len_tmp++;
    memcpy(num+(3-r), num_tmp, sizeof(char) * r);
    memcpy(buff+len_tmp, num, sizeof(char) * 3);
    len_tmp += 3;
    memcpy(buff+len_tmp, tail, sizeof(char) * TAIL_LEN);
    len_tmp += 3;

    if(send(infos->tcp_socket, buff, sizeof(char) * len_tmp, 0) < 0) {
        perror("Error from send() in mv_com()");
        return -2;
    }

    //reply
    r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
    if(r < 0) {
        perror("Error from recv() in mv_com()");
        return -2;
    }

    //verif protocol
    len_tmp = 16;
    if(r == len_tmp) { //MOVE!
        if(verif_code_tail(buff, r, "MOVE!", "***") < 0) {
            printf("Protocol error from mv_com()\n");
            return -1;
        }

        memcpy(x, buff+6, sizeof(char) * POS_LEN);
        memcpy(y, buff+10, sizeof(char) * POS_LEN);
        printf("%sMove performed. (x,y) = (%s,%s)%s\n", YELLOW, x, y, RESET);
        return 0;

    }
    else if(r == len_buff) { //MOVEF
        if(verif_code_tail(buff, r, "MOVEF", "***") < 0) {
            printf("Protocol error from mv_com()\n");
            return -1;
        }
        
        memcpy(x, buff+6, sizeof(char) * POS_LEN);
        memcpy(y, buff+10, sizeof(char) * POS_LEN);
        memcpy(pts, buff+14, sizeof(PTS_LEN));
        printf("%sMove performed, and you found a ghost (current points: %s) ! (x,y) = (%s,%s)%s\n",
        YELLOW, pts, x, y, RESET);
        return 0;

    }
    else if(r == (CODE_LEN+TAIL_LEN)) { //stop
        return gobye(buff);

    }
    else {
        printf("Protocol error from mv_com()\n");
        return -1;
    }
}


int pl_game_com(struct game_infos *infos) {
    char buff[CODE_LEN + TAIL_LEN + 1] = "GLIS?***";

    //req
    if(send(infos->tcp_socket, buff, sizeof(char) * (CODE_LEN+TAIL_LEN), 0) < 0) {
        perror("Error from send() in pl_game_com()");
        return -2;
    }
    return print_players_game(infos);
}


int print_players_game(struct game_infos *infos) {
    int len_buff = 30;
    int r = 0;
    uint8_t nb = 0;
    char buff[len_buff];
    char id[ID_LEN+1];
    char x[POS_LEN+1];
    char y[POS_LEN+1];
    char pts[PTS_LEN+1];
    id[ID_LEN] = '\0';
    x[POS_LEN] = '\0';
    y[POS_LEN] = '\0';
    pts[PTS_LEN] = '\0';
    
    //reply
    r = recv(infos->tcp_socket, buff, sizeof(char) * 10, 0);
    if(r < 0) {
        perror("Error from recv() in print_players_game()");
        return -2;
    }

    //verif code
    if(r == 10) { //ok
        if(verif_code_tail(buff, r, "GLIS!", "***") < 0) {
            printf("Protocol error from print_players_game()\n");
            return -1;
        }

        nb = buff[6];

    }
    else if(r == CODE_LEN+TAIL_LEN) { //stop
        return gobye(buff);

    }
    else { //err
        printf("Protocol error from print_players_game()\n");
        return -1;
    }

    //rec + print
    printf("%sThe game have %d players:%s\n", YELLOW, (int)nb, RESET);
    for(uint8_t i = 0; i < nb; i++) {
        r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
        if(r < 0) {
            perror("Error from recv() in print_players_game()");
            return -2;
        }
        
        //verif
        if(r == len_buff) { //ok
            if(verif_code_tail(buff, r, "GPLYR", "***") < 0) {
                printf("Protocol error from print_players_game()\n");
                return -1;
            }
            //rec datas
            memcpy(id, buff+(CODE_LEN+1), sizeof(char) * ID_LEN);
            memcpy(x, buff+(CODE_LEN+ID_LEN+2), sizeof(char) * POS_LEN);
            memcpy(y, buff+(CODE_LEN+ID_LEN+POS_LEN+3),sizeof(char) * POS_LEN);
            memcpy(pts, buff+(CODE_LEN+ID_LEN+POS_LEN+POS_LEN+4), sizeof(char) * PTS_LEN);
            
            printf("\t%sPlayer: %s, pts: %s. (x,y) = (%s,%s)%s\n",
            YELLOW, id, pts, x, y, RESET);
            
        }
        else if(r == CODE_LEN+TAIL_LEN) { //stop
            return gobye(buff);

        }
        else { //err
            printf("Protocol error from print_players_game()\n");
            return -1;
        }
    }
    return 0;
}


int am_com(struct game_infos *infos, char *msg) {
    int len_msg = strlen(msg);
    int len_tmp = CODE_LEN;
    int len_buff = 9 + len_msg;
    int r = 0;
    char buff[len_buff];

    memcpy(buff, "MALL?", sizeof(char) * len_tmp);
    buff[len_tmp] = ' ';
    len_tmp++;
    memcpy(buff+len_tmp, msg, sizeof(char) * len_msg);
    len_tmp += len_msg;
    memcpy(buff+len_tmp, "***", sizeof(char) * TAIL_LEN);

    if(send(infos->tcp_socket, buff, len_buff, 0) < 0) {
        perror("Error from send() in am_com()");
        return -2;
    }
    len_tmp = CODE_LEN + TAIL_LEN;
    
    //verif
    r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
    if(r < 0) {
        perror("Error from recv() in am_com()");
        return -2;
    }
    if(r != len_tmp) {
        printf("Protocol error from am_com()\n");
        return -1;
    }

    buff[len_tmp] = '\0';
    if(strcmp("MALL!***", buff) == 0) {
        printf("%sMessage has been sent !%s\n", YELLOW, RESET);
        return 0;
    }
    else if(strcmp("GOBYE***", buff) == 0) {
        running = 0;
        printf("%sGame has ended%s\n", YELLOW, RESET);
        return 0;
    }
    else {
        printf("Protocol error from am_com()\n");
        return -1;
    }
}


int pm_com(struct game_infos *infos, char *msg, char *id) {
    int len_msg = strlen(msg);
    int len_tmp = CODE_LEN;
    int len_buff = 10 + ID_LEN + len_msg;
    int r = 0;
    char buff[len_buff];

    memcpy(buff, "SEND?", sizeof(char) * len_tmp);
    buff[len_tmp] = ' ';
    len_tmp++;
    memcpy(buff+len_tmp, id, sizeof(char) * ID_LEN);
    len_tmp += ID_LEN;
    buff[len_tmp] = ' ';
    len_tmp++;
    memcpy(buff+len_tmp, msg, sizeof(char) * len_msg);
    len_tmp += len_msg;
    memcpy(buff+len_tmp, "***", sizeof(char) * TAIL_LEN);

    if(send(infos->tcp_socket, buff, len_buff, 0) < 0) {
        perror("Error from send() in pm_com()");
        return -2;
    }
    len_tmp = CODE_LEN + TAIL_LEN;

    //verif
    r = recv(infos->tcp_socket, buff, sizeof(char) * len_buff, 0);
    if(r < 0) {
        perror("Error from recv() in pm_com()");
        return -2;
    }
    if(r != len_tmp) {
        printf("Protocol error from pm_com()\n");
        return -1;
    }

    buff[len_tmp] = '\0';
    if(strcmp("SEND!***", buff) == 0) {
        printf("%sMessage has been sent !%s\n", YELLOW, RESET);
        return 0;
    }
    else if(strcmp("NSEND***", buff) == 0) {
        printf("%sMessage was not sent... Is the id correct ?%s\n", YELLOW, RESET);
        return 0;
    }
    else if(strcmp("GOBYE***", buff) == 0) {
        running = 0;
        printf("%sGame has ended%s\n", YELLOW, RESET);
        return 0;
    }
    else {
        printf("Protocol error from pm_com()\n");
        return -1;
    }
}


int lg_com(struct game_infos *infos) {
    int r = 0;
    char buff[CODE_LEN+TAIL_LEN+1] = "IQUIT***";

    if(send(infos->tcp_socket, buff, (CODE_LEN+TAIL_LEN), 0) < 0) {
        perror("Error from send() in lg_com()");
        return -2;
    }

    //rec
    r = recv(infos->tcp_socket, buff, (CODE_LEN+TAIL_LEN), 0);
    if(r < 0) {
        perror("Error from recv() in lg_com()");
        return -2;
    }

    //check
    if(r == (CODE_LEN+TAIL_LEN)) { //GOBYE
        return gobye(buff);
    }
    else { //err
        printf("Protocol error from lg_com()\n");
        return -1;
    }
}


int ub_com(struct game_infos *infos) {
    int r = 0;
    char buff[CODE_LEN+TAIL_LEN+1] = "EXPL?***";

    if(send(infos->tcp_socket, buff, (CODE_LEN+TAIL_LEN), 0) < 0) {
        perror("Error from send() in ub_com()");
        return -2;
    }

    //rec
    r = recv(infos->tcp_socket, buff, (CODE_LEN+TAIL_LEN), 0);
    if(r < 0) {
        perror("Error from recv() in ub_com()");
        return -2;
    }
    if(r != (CODE_LEN+TAIL_LEN)) {
        printf("Protocol error from ub_com()\n");
        return -1;
    }

    //check
    if(strcmp("EXPL!***", buff) == 0) {
        printf("%s Hide, it's going to blow...\nBOOM !%s\n",YELLOW, RESET);
        return 0;
    }
    else if(strcmp("EXPLN***", buff) == 0) {
        printf("%sDon't burn your hands, you don't have any bombs in stock !%s\n", YELLOW, RESET);
        return 0;
    }
    else if(strcmp("GOBYE***", buff) == 0) {
        running = 0;
        printf("%sGame has ended\n%s", YELLOW, RESET);
        return 0;
    }
    else { //err
        printf("Protocol error from ub_com()\n");
        return -1;
    }
}


int ur_com(struct game_infos *infos) {
    int len_buff = (CODE_LEN + TAIL_LEN);
    char buff[len_buff];

    //req
    memcpy(buff, "RADA?***", len_buff);
    if(send(infos->tcp_socket, buff, sizeof(char) * len_buff, 0) < 0) {
        perror("Error from send() in ur_com()");
        return -2;
    }
    return print_radar(infos);
}


int print_radar(struct game_infos *infos) {
    int len_tmp = 12;
    int r, len_buff = 0;
    uint8_t n, y = 0;
    char buff_tmp[len_tmp+1];

    //rec
    r = recv(infos->tcp_socket, buff_tmp, sizeof(char) * len_tmp, 0);
    if(r < 0) {
        perror("Error from recv() in print_radar()");
        return -2;
    }
    buff_tmp[r] = '\0';
    
    //check
    if(r == len_tmp) { //RADA!
        if(verif_code_tail(buff_tmp, r, "RADA!", "***") < 0) {
            printf("Protocol error from print_radar()\n");
            return -1;
        }
        n = buff_tmp[6];
        y = buff_tmp[8];
    }
    else if(strcmp("RADAN***", buff_tmp) == 0) {
        printf("%sYou don't have a radar in stock...%s\n", YELLOW, RESET);
        return 0;
    }
    else if(r == (CODE_LEN+TAIL_LEN)) { //GOBYE
        return gobye(buff_tmp);
    }
    else {
        printf("Protocol error from print_radar()\n");
        return -1;
    }

    len_buff = CODE_LEN + TAIL_LEN + y + 1;
    char buff[len_buff];

    //print
    printf("%sRadar infos:%s\n", YELLOW, RESET);
    for(uint8_t i = 0; i < n; i++) {
        //rec
        r = recv(infos->tcp_socket, buff, len_buff, 0);
        if(r < 0) {
            perror("Error from recv() in print_radar()");
            return -2;
        }

        if(r == len_buff) {
            if(verif_code_tail(buff, r, "CASES", "***") < 0) {
                printf("Protocol error from print_radar()\n");
                return -1;
            }

            buff[CODE_LEN + y + 1] = '\0';
            printf("%s%s%s\n", YELLOW, buff+(CODE_LEN+1), RESET);
        }
        else if(r == (CODE_LEN + TAIL_LEN)) {
            return gobye(buff);
        }
        else {
            printf("Protocol error from print_radar()\n");
        return -1;
        }
    }

    return 0;
}
