#include "protocol.h"

int run;


void handler(int s)
{
	s = s; //warning compilation
	run = 0;
	printf("SIGINT\n");
}

void reply(int sock, int answer) {
	send(sock, &answer, BYTE_LEN, 0);
}

int main(int argc, char *argv[])
{
    uint64_t vote_ete = 0, vote_hiver = 0;
    int sock, sock_cli, tmp;
	socklen_t socksz;
    short port;
    struct sockaddr_in saddr, saddr_2;
    uint8_t req[REQ_LEN];
	char name[NAME_MAX_LEN + 1];


    if (argc != 2) {
        fprintf(stderr, "expected args: port\n");
		return EXIT_FAILURE;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        return EXIT_FAILURE;
    }


    port = (short)strtol(argv[1], NULL, 10);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("bind");
		return EXIT_FAILURE;
    }

	if (listen(sock, PENDING_CLI)) {
		perror("listen");
		return EXIT_FAILURE;
	}

	signal(SIGINT, handler);

	socksz = sizeof(saddr_2);
	run = 1;
	
	struct pollfd p[1];
	p[0].fd = sock;
	p[0].events = POLLIN;

	while (run) {
		tmp = poll(p, 1, 2000);
		if (tmp < 0) //en cas de SIGINT, on sort de l'espace sys, on execute le handler et on vient dans ce cas
			break;
		else if (tmp == 0)
			continue;

		sock_cli = accept(sock, (struct sockaddr *)&saddr_2, &socksz);
		if (recv(sock_cli, req, REQ_LEN, 0) <= 0) {
			perror("ignore client - problem");
			continue;
		}

		memcpy(&tmp, req, BYTE_LEN);
		if (be32toh(tmp) != VOTE) { //mauvaise commande
			reply(sock_cli, htobe32(ERROR));
			goto cleanup;
		}
		else {
			memcpy(&tmp, req + BYTE_LEN, BYTE_LEN);
			tmp = be32toh(tmp);
			if (tmp != ETE && tmp != HIVER) { //si pas un vote acceptable
				reply(sock_cli, htobe32(ERROR));
				goto cleanup;
			}
			
			(tmp == ETE)? ++vote_ete : ++vote_hiver; //quel vote ?
			memcpy(name, req + (BYTE_LEN * 2), NAME_MAX_LEN);
			name[NAME_MAX_LEN] = '\0';
			printf("%s - hiver = %lu | ete = %lu\n", name, vote_hiver, vote_ete);
			reply(sock_cli, htobe32(ACCEPT_VOTE));
		}

		cleanup: //fin du client
		shutdown(sock_cli, 2);
		close(sock_cli);
	}

	printf("server exit\n");
	close(sock);
    return EXIT_SUCCESS;
}
