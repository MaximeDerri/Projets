#include "txt_channel.h"


void *listen_udp_mc(void *ptr) {
    struct game_infos *infos = (struct game_infos *)ptr;
    int fd_max;
    char *sep = "==================================\n";
    struct timeval tv;
    fd_set rdfs;

    if(fcntl(infos->udp_socket, F_SETFL, O_NONBLOCK) < 0) {
        perror("Error from fcntl() - liste_udp_mc()");
        pthread_exit(0);
    }
    if(fcntl(infos->mc_socket, F_SETFL, O_NONBLOCK) < 0) {
        perror("Error from fcntl() - liste_udp_mc()");
        pthread_exit(0);
    }

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    if(write_txt_channel(infos->txt_channel, sep, sizeof(char)*strlen(sep)) < 0)
        pthread_exit(0);

    while(running) {
        //FD_COPY seems to not be accessible for me
        fd_max = 0;
        FD_ZERO(&rdfs);
        FD_SET(infos->udp_socket, &rdfs);
        if(fd_max < infos->udp_socket)
            fd_max = infos->udp_socket;
        FD_SET(infos->mc_socket, &rdfs);
        if(fd_max < infos->mc_socket)
            fd_max = infos->mc_socket;

        //selection
        if(select(fd_max+1, &rdfs, NULL, NULL, &tv) < 0) {
            perror("Error from select in listen_udp_mc()");
            pthread_exit(0);
        }

        if(FD_ISSET(infos->udp_socket, &rdfs)) {
            if(check_udp_sock(infos) < -1) { //if err
                pthread_exit(0);
            }
        }
        if(FD_ISSET(infos->mc_socket, &rdfs)) {
            if(check_mc_sock(infos) < -1) { //if err
                pthread_exit(0);
            }
        }
    } //end while

    write_txt_channel(infos->txt_channel, sep, sizeof(char)*strlen(sep));
    pthread_exit(0);
}


//no mutex because thread use non-blocking udp socket
int write_txt_channel(int channel, char *buff, int length) {
    if(write(channel, buff, length) < 0) {
        perror("Error from write in write_txt_channel()");
        return -1;
    }
    return 0;
}


int prep_msg_and_send(int channel, char *buff, int len, char *id, char *dest) {
    char message[TXT_CHANNEL_MAX_LEN];
    struct tm tm;
    time_t t = time(NULL);

    int len_tmp = len-(CODE_LEN + ID_LEN + 5);
    if(len_tmp <= 0) //message have length 0 but not an error
        return 0;

    //extract val
    char msg_tmp[len_tmp + 1];
    msg_tmp[len_tmp] = '\0';
    memcpy(msg_tmp, buff+(CODE_LEN+ID_LEN+2), sizeof(char) * len_tmp);
    tm = *localtime(&t);

    len_tmp = sprintf(message, "%s[%s -> %s | %02d:%02d]:\n\t%s%s\n",
        BLUE, id, dest, tm.tm_hour, tm.tm_min, msg_tmp, RESET);
    if(len_tmp < 0) {
        perror("Error from sprintf() in prep_msg_and_send");
        return -2;
    }
    return write_txt_channel(channel, message, len_tmp);

}


int check_mc_sock(struct game_infos *infos) {
    int r = 0;
    int len_buff = 218;
    int len_tmp = 16; //GHOST_x_y+++ -> the smallest req length
    char buff[len_buff];
    char req[CODE_LEN+1];
    char id[ID_LEN+1];
    char message[TXT_CHANNEL_MAX_LEN];
    char x[POS_LEN+1];
    char y[POS_LEN+1];
    char pts[PTS_LEN+1];
    char *tail = "+++";
    struct tm tm;
    time_t t = time(NULL);
    x[POS_LEN] = '\0';
    y[POS_LEN] = '\0';
    pts[PTS_LEN] = '\0';
    id[ID_LEN] = '\0';
    message[0] = '\0';

    r = recv(infos->mc_socket, buff, len_buff, 0);
    if (r < 0) {
        perror("Error from recv() in check_mc_sock()");
        return -2;
    }
    if(r < len_tmp) { //the smallest req length for multicast - no need to continue
        printf("Protocol error from check_mc_sock()\n");
        return -1;
    }

    //verif
    memcpy(req, buff+(r-TAIL_LEN), sizeof(char) * TAIL_LEN);
    req[TAIL_LEN] = '\0';
    if(verif_protocol(tail, req) != 0)
        return -1;

    memcpy(req, buff, sizeof(char) * CODE_LEN);
    req[CODE_LEN] = '\0';

    // 'switch' + more accurate checks
    if(strcmp("GHOST", req) == 0) {
        //len_tmp ok
        if(r != len_tmp) { //length expected
            printf("Protocol error from check_mc_sock()\n");
            return -1;
        }

        //extract val
        memcpy(x, buff+(CODE_LEN+1), sizeof(char) * POS_LEN);
        memcpy(y, buff+(CODE_LEN+POS_LEN+2), sizeof(char) * POS_LEN);
        tm = *localtime(&t);

        len_tmp = sprintf(message, "%s[SERV -> ALL | %02d:%02d]:\n\tA ghost moves !\n(x,y) = (%s,%s)%s\n", YELLOW, tm.tm_hour, tm.tm_min, x, y, RESET);
        if(len_tmp < 0) {
            perror("Error from sprintf() in check_mc_sock()");
            return -2;
        }
        return write_txt_channel(infos->txt_channel, message, len_tmp);

    }
    else if(strcmp("SCORE", req) == 0) {
        len_tmp = 30;
        if(r != len_tmp) { //length expected
            printf("Protocol error from check_mc_sock()\n");
            return -1;
        }

        //extract val
        memcpy(id, buff+(CODE_LEN+1), sizeof(char) * ID_LEN);
        memcpy(pts, buff+(CODE_LEN+ID_LEN+2), sizeof(char) * PTS_LEN);
        memcpy(x, buff+(CODE_LEN+ID_LEN+PTS_LEN+3), sizeof(char) * POS_LEN);
        memcpy(y, buff+(CODE_LEN+ID_LEN+PTS_LEN+POS_LEN+4), sizeof(char) * POS_LEN);
        tm = *localtime(&t);

        len_tmp = sprintf(message, "%s[SERV -> ALL | %02d:%02d]:\n\tA ghost has been caught by %s (currents points: %s) !\n(x,y) = (%s,%s)%s\n", YELLOW, tm.tm_hour, tm.tm_min, id, pts, x, y, RESET);
        if(len_tmp < 0) {
            perror("Error from sprintf() in check_mc_sock()");
            return -2;
        }
        return write_txt_channel(infos->txt_channel, message, len_tmp);

    }
    else if(strcmp("ENDGA", req) == 0) {
        len_tmp = 22;
        if(r != len_tmp) { //length expected
            printf("Protocol error from check_mc_sock()\n");
            return -1;
        }
        running = 0; //thread will stop at the end of the while iteration

        //extract val
        memcpy(id, buff+(CODE_LEN+1), sizeof(char) * ID_LEN);
        memcpy(pts, buff+(CODE_LEN+ID_LEN+2), sizeof(char) * PTS_LEN);
        tm = *localtime(&t);

        len_tmp = sprintf(message, "%s[SERV -> ALL | %02d:%02d ]:\n\tThe game finally end !\n%s win with %s points%s\n",
            YELLOW, tm.tm_hour, tm.tm_min, id, pts, RESET);
        if(len_tmp < 0) {
            perror("Error from sprintf() in check_mc_sock()");
            return -2;
        }
        return write_txt_channel(infos->txt_channel, message, len_tmp);
    
    }
    else if(strcmp("MESSA", req) == 0) {
        len_tmp = 18; //min length (because of mess)
        if(r < len_tmp) {
            printf("Protocol error from check_mc_sock()\n");
            return -1;
        }
        memcpy(id, buff+(CODE_LEN+1), sizeof(char) * ID_LEN);
        return prep_msg_and_send(infos->txt_channel, buff, r, id, "ALL");

    }
    else if(strcmp("BOMB!", req) == 0) {
        len_tmp = 25;
        if(r != 25) {
            printf("Protocol error from check_mc_sock()\n");
            return -1;
        }
        tm = *localtime(&t);

        memcpy(id, buff+(CODE_LEN+1), sizeof(char) * ID_LEN);
        memcpy(x, buff+(CODE_LEN+ID_LEN+2), sizeof(char) * POS_LEN);
        memcpy(y, buff+(CODE_LEN+ID_LEN+POS_LEN+3), sizeof(char) * POS_LEN);

        len_tmp = sprintf(message, "%s[SERV -> ALL | %02d;%02d ]:\n\tYou hear something burning... %s is using a bomb !\n(x,y) = (%s,%s)%s\n", YELLOW, tm.tm_hour, tm.tm_min, id, x, y, RESET);
        if(len_tmp < 0) {
            perror("Error from sprintf() in check_mc_sock()");
            return -2;
        }
        return write_txt_channel(infos->txt_channel, message, len_tmp);
        
    }
    else {
        printf("Protocol error from check_mc_sock()\n");
        return -1;
    }
}


int check_udp_sock(struct game_infos *infos) {
    int r = 0;
    int len_buff = 218;
    int len_tmp = 18; //min length
    char buff[len_buff];
    char req[CODE_LEN+1];
    char id[ID_LEN+1];
    char *tail = "+++";
    id[ID_LEN] = '\0';

    r = recv(infos->udp_socket, buff, len_buff, 0);
    if (r < 0) {
        perror("Error from recv() in check_udp_sock()");
        return -2;
    }
    if(r < len_tmp) { //the smallest req length for multicast - no need to continue
        printf("Protocol error from check_udp_sock()\n");
        return -1;
    }

    //verif
    memcpy(req, buff+(r-TAIL_LEN), sizeof(char) * TAIL_LEN);
    req[TAIL_LEN] = '\0';
    if(verif_protocol(tail, req) != 0)
        return -1;

    memcpy(req, buff, sizeof(char) * CODE_LEN);
    req[CODE_LEN] = '\0';


    if(strcmp("MESSP", req) == 0) {
        memcpy(id, buff+(CODE_LEN+1), sizeof(char) * ID_LEN);
        return prep_msg_and_send(infos->txt_channel, buff, r, id, infos->id);

    }
    else {
        printf("Protocol error from check_udp_sock()\n");
        return -1;
    }
}
