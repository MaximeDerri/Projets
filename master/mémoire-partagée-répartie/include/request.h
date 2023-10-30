#ifndef REQUEST_H
#define REQUEST_H

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

#define MESSLEN 6

/**
 * extrait l'entier de la requete, en fonction de la taille spécifié dans la config
 * prend en parametre:
 * un pointeur sur l'entier qui contiendra le resultat, la contenu de la requete (sans opcode), et l'offset dans la requete
 * le nombre d'octets a extraire sera deduit de la config
*/
#ifndef ARCHI
    void extract_integer(uint64_t ret, uint32_t offset, uint8_t *buff);
#endif
#ifdef ARCHI
#if ARCHI == 32
    void extract_integer(uint32_t ret, uint32_t offset, uint8_t *buff);
#else
    void extract_integer(uint64_t ret, uint32_t offset, uint8_t *buff);
#endif
#endif

/** 
 * recupere du socket size octets et les stock dans buff
 * retourne -1 en cas d'erreur,
 * retourne 0 si la connexion est close ou 1 si tout est correct
*/
int extract_request(uint8_t *buff, uint32_t size, uint32_t sock);

/**
 * extrait dans et stock dans opcode et dans req le contenu de la requete
 * nb_bytes_config est le nombre d'octets definit par la configuration (recu par sizeof ici)
 * retourne -2 si opcode pas correct, -1 si erreur,
 * 0 si la connexion est close ou 1 si tout est correct
 * // opcode doit etre de taille 6 au moins ('\0' est ajoute)
*/
int rec_request_userfaultfd(uint8_t *buff, char *opcode, uint32_t sock, uint32_t nb_bytes_config);


#endif