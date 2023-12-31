/* A simple server in the internet domain using TCP The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <arpa/inet.h>


void error(const char *msg)
{
        perror(msg);
        exit(1);
}

/*
On a deja vue ca en PSCR
*/
int main(int argc, char *argv[])
{
        int sockfd, newsockfd, portno;
        socklen_t clilen;
        char buffer[256];
        struct sockaddr_in serv_addr, cli_addr;
        int n;

        if (argc < 2) {
                fprintf(stderr, "ERROR, no port provided\n");
                exit(1);
        }

        // 1) on crée la socket, SOCK_STREAM signifie TCP

        sockfd = socket(AF_INET, SOCK_STREAM, 0); //cree un socket TCP
        if (sockfd < 0)
                error("ERROR opening socket");

        // 2) on réclame au noyau l'utilisation du port passé en paramètre 
        // INADDR_ANY dit que la socket va être affectée à toutes les interfaces locales

        bzero((char *) &serv_addr, sizeof(serv_addr)); //met a 0 les octets
        portno = atoi(argv[1]); //char * -> entier
        serv_addr.sin_family = AF_INET; //IPv4
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno); //mets en big le port (s = short)
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) //lier le socket et la struct definie
                error("ERROR on binding");


        // On commence à écouter sur la socket. Le 5 est le nombre max
        // de connexions pendantes

        listen(sockfd, 5); //initier l'ecoute sur le socket serveur, 5 clients en attente max
        while (1) { //comme on boucle en continue, on pourrait faire une variable globale qu'on change à la réception d'un signal (utilisation de signal() ou sigaction)
                newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); //accepter un client et retourne un socket pour communiquer avec lui
                if (newsockfd < 0)
                    error("ERROR on accept");

                bzero(buffer, 256);
                n = read(newsockfd, buffer, 255); //recupere les donnees et les places dans le buffer (d'autres fct existes: recv, recvfrom)
                if (n < 0)
                    error("ERROR reading from socket");

                printf("Received packet from %s:%d\nData: [%s]\n\n",
                       inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), //affiche l'adresse sous forme d'entier en char * avec "a:b:c:d" | remet le port dans l'endian de l'host
                       buffer); //IPv4 -> 4 octets | IPv6 -> 16 octets

                close(newsockfd); //fermer la connexion
        }

        close(sockfd); //fermer le socket du serveur
        return 0;
}
