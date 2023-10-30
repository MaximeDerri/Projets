#include "request.h"


#ifndef ARCHI
void extract_integer(uint64_t ret, uint32_t offset, uint8_t *buff) {
#endif
#ifdef ARCHI
#if ARCHI == 32
void extract_integer(uint32_t ret, uint32_t offset, uint8_t *buff) {
#else
void extract_integer(uint64_t ret, uint32_t offset, uint8_t *buff) {
#endif
#endif
	//------------------------
	ret = 0;
	for(int i = 0; i < (int)sizeof(ret); ++i) {
		ret = (ret << 8) | buff[(offset+i)]; // i=0: (0 << 8) == 0
	}
}


/* extraire requete */
int extract_request(uint8_t *buff, uint32_t size, uint32_t sock) {
	if(buff == NULL)
		return -1;
	
    int32_t to_read = size;
    int32_t tmp = 0;

    while(to_read > 0) {
        tmp = recv(sock, buff+(size - to_read), to_read, 0);
        if(tmp < 0) {
            perror("error on recv() - extract_request()");
            return -1;
        }
        if(tmp == 0) {
            fprintf(stderr, "the connection is detected as closed - extract_request()\n");
            return 0;
        }

        to_read -= tmp;
    }
    return 1;
}


//recuperer la requete pour le thread userfaultfd
int rec_request_userfaultfd(uint8_t *buff, char *opcode, uint32_t sock, uint32_t nb_bytes_config) {
	//on peut recevoir: RELEA, INVAL, PAGES, ERROR
	if(buff == NULL || opcode == NULL)
		return -2;
	
	int32_t tmp = 0;
	int32_t size = 0;

	//la fonction extrait la requete en deux parties, pour ne pas grignotter un morceau d'une autre requete dans le buffer
	//----------------------------------------------------------

    //recup requete
    tmp = extract_request((uint8_t *)opcode, MESSLEN, sock);
    if(tmp <= 0) { //erreur ou socket clos
            return tmp;
    }
	opcode[MESSLEN] = '\0';

	//verifier l'opcode et extraire en fonction le reste de la requete
	if(strcmp(opcode, "RELEA") == 0 || strcmp(opcode, "ERROR") == 0) { //ici, on a pas d'octets a extraire
		return 1;
	}
	else {
		//determiner la taille
		if(strcmp(opcode, "INVAL") == 0) {
			size = nb_bytes_config*2;
		}
		else if(strcmp(opcode, "PAGES") == 0) {
			size = nb_bytes_config + sysconf(_SC_PAGE_SIZE);
		}
		else { //opcode non correct
			return -2;
		}

        tmp = extract_request(buff, size, sock);
        if(tmp <= 0) { //erreur ou socket clos
            return tmp;
        }
	}

	return 1;
}
