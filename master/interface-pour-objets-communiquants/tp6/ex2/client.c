#include "protocol.h"


int main(int argc, char *argv[])
{
    struct sockaddr_in saddr;
    int sock, tmp, tmp_2;
    short port;
    uint8_t req[(BYTE_LEN * 2) + NAME_MAX_LEN];

    if (argc != 5) {
        fprintf(stderr, "expected args: ip port name vote\n");
        return EXIT_FAILURE;
    }

    port = (short)strtol(argv[2], NULL, 10);
    memset(&saddr, 0, sizeof(saddr));

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    inet_pton(AF_INET, argv[1], &saddr.sin_addr.s_addr);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if (connect(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("connect");
        return EXIT_FAILURE;
    }

    memset(req, 0, REQ_LEN * sizeof(uint8_t));
    tmp = htobe32(VOTE);
    memcpy(req, &tmp, BYTE_LEN);
    
    if (strcmp(argv[4], "ete") == 0)
        tmp = htobe32(ETE);
    else
        tmp = htobe32(HIVER); //par defaut, sinon on pourrait implementer un vote blanc ou sortir en erreur (reste dans la meme idee que le reste du code...)

    memcpy(req + BYTE_LEN, &tmp, BYTE_LEN);
    tmp_2 = ((strlen(argv[3])) > NAME_MAX_LEN)? NAME_MAX_LEN : strlen(argv[3]);
    memset(req + (BYTE_LEN*2), ' ', NAME_MAX_LEN); //pad pour char manquant
    memcpy(req + (BYTE_LEN*2), argv[3], tmp_2);

    tmp = send(sock, req, REQ_LEN, 0);
    if (tmp <= 0) { //0 = close
        perror("socket was closed");
        return EXIT_FAILURE;
    }

    tmp = recv(sock, &tmp_2, sizeof(tmp_2), 0);
    if (tmp < 0) { //0 = close
        perror("recv");
        return EXIT_FAILURE;
    }

    printf("result: %s\n", (be32toh(tmp_2) == ACCEPT_VOTE)? "take in account" : "error");

    shutdown(sock, 2);
    close(sock);
    return EXIT_SUCCESS;
}
