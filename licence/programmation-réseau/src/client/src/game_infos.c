#include "game_infos.h"


//string const
const char *PATH_PIPE = "/tmp/ghostlab/textual-channel";
const char *RED = "\033[31m";    //err from serv (ex: start but user is not in a party)
const char *BLUE = "\033[34m";   //msg
const char *YELLOW = "\033[33m"; //infos serv (ex: ghost move, points)
const char *RESET = "\033[0m";


int running = 1; //to indicate the thread and in_game() to stop


int set_infos(struct game_infos *infos, char *id, char *tcp_port, uint16_t udp_port) {
    //pipe for text channel
    struct stat st;
    if(stat(PATH_PIPE, &st) != 0) { //if the pipe is not found
        mode_t mode = umask(0); //to avoid Permission Denied error
        if((mkdir("/tmp/ghostlab", 0700) < 0) && errno != EEXIST) { //check if the dir exist (but not the pipe - stat())
            perror("Error from mkdir() before mkfifo()");
            umask(mode);
            return -1;
        }
        if(mkfifo(PATH_PIPE, 0700) < 0) {
            perror("Error from mkfifo()");
            umask(mode);
            return -1;
        }
        umask(mode);
    }
    infos->txt_channel = -1;


    //tcp
    struct addrinfo *f_info;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int r = 0;

    if(LOCALHOST == 1)
        r = getaddrinfo("localhost", tcp_port, &hints, &f_info);
    else
        r = getaddrinfo("lulu.informatique.univ-paris-diderot.fr", tcp_port, &hints, &f_info);
    if(r == 0) {
        struct addrinfo *inf = f_info;
        struct sockaddr *ad;
        if(inf != NULL) {
            ad = inf->ai_addr;
            infos->tcp_addr = *(struct sockaddr_in *)ad;
            freeaddrinfo(f_info);
        }
        else { //f_info is null, no need to free
            perror("Error NULL from getaddrinfo()");
            return -1;
        }
    }
    else {
        perror("Error from getaddrinfo()");
        return -1;
    }

    infos->tcp_socket = socket(PF_INET, SOCK_STREAM, 0);
    if(infos->tcp_socket == -1) {
        perror("Error from socket() for tcp_socket");
        return -1;
    }


    //udp
    infos->udp_addr.sin_family = AF_INET;
    infos->udp_addr.sin_port= htons(udp_port);
    infos->udp_addr.sin_addr.s_addr =  htonl(INADDR_ANY);
    infos->udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if(infos->udp_socket == -1) {
        perror("Error from socket() for udp_socket");
        return -1;
    }


    //id - len already check with check_args()
    memcpy(infos->id, id, sizeof(char) * ID_LEN);
    infos->id[ID_LEN] = '\0';

    infos->is_register = 0; //used for 'START***', to don't send it and be blocked

    return 0;
}


void clear_infos(struct game_infos *infos) {
    close(infos->tcp_socket);
    if(infos->txt_channel >= 0)
        close(infos->txt_channel);
}