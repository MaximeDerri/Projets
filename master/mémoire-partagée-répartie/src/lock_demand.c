#include "lock_demand.h"

/*
    Args:
        value: le message qu'on cherche a recevoir pour terminer la verif, soit OKLOC (pour les locks)
            soit OKLUN (pour les unlocks)
        slv: le pointeur de l'esclave concerne par la verification 

    Valeur de retour:
        -1 en cas d'erreur, sinon 0
    
    Fonction qui permet de valider un lock fait par un esclave, une fois que la verification est effectue et 
    que cette fonction renvoie 0, le maitre a bien lock ou unlock les pages concerne par notre demande et de 
    continuer a gerer les requetes d'invalidations
*/
int8_t verif(char* value, struct dsm_slave *slv){//verification message de retour de socket = value, 1 reussi, -1 erreur

	if(slv == NULL)
		return -1;
	if(slv->ptr == NULL || slv->run == 0 || slv->uf_fd < 0) {
		slv->run = 0;
		return -1;
	}
	
	uint32_t max_buffer_size = sizeof(slv->size) + PAGE_SIZE; // size : deppend de la config et du protocol
	uint8_t buff[max_buffer_size];						    // max_buffer_size --> plus grande requete ici c'est celle d'opcode PAGES
	char opcode[MESSLEN]; //opcode de la requete

#ifndef ARCHI
    uint64_t offset = 0;
	uint64_t nb_page = 0;
#endif
#ifdef ARCHI
#if ARCHI == 32
    uint32_t offset = 0;
	uint32_t nb_page = 0;
#else
    uint64_t offset = 0;
	uint64_t nb_page = 0;
#endif
#endif

    do {
        int rec = recv(slv->co.socket, opcode, MESSLEN, 0);

        if(rec == -1) {
            perror("read lock");
            return -1; // renvoie toujours -1 en cas d'erreur
        }

        if(strcmp(opcode, RELEA) == 0 ||  strcmp(opcode, ERROR) == 0) {
            slv->run = 0;
            return -1;
        } else if (strcmp(opcode, INVAL) == 0) {

            if (recv(slv->co.socket, buff, sizeof(offset) + sizeof(nb_page), 0) < 0) {
                perror("recv verif INVAL");
            }

            memcpy(&offset, buff, sizeof(offset));
            memcpy(&nb_page, buff + sizeof(offset), sizeof(nb_page));

            //remettre sous endian de l'host
            if(sizeof(offset) == 8) { //64 bits
                offset = be64toh(offset);
                nb_page = be64toh(nb_page);
            }
            else {
                offset = be32toh(offset);
                nb_page = be32toh(nb_page);
            }

            // Offset de la page 
            offset = (offset/PAGE_SIZE) * PAGE_SIZE;

            //changer la protection
            if(alter_protect_page(PROT_NONE, (void*) ((unsigned long)slv->ptr+offset), nb_page*PAGE_SIZE) < 0) {
                slv->run = 0;
                return -1;
            }

        } else if (strcmp(opcode,value) != 0){
            /* correspondrait a recevoir une requete PAGES, mais celle ci est provoqué par la gestion des pagefault,
            on peut donc se dire qu'il y a un pb. De plus, si c'est bien une reponse a une requete initié par
            la gestion des pages, elle ne devrait pas être traitée ici et cela serait une erreur dans le code... */
            
            fprintf(stderr, "unexpected opcode in slave verif = %s\n", opcode);
            slv->run = 0;
            return -1;
        }
    } while (strcmp(opcode,value) != 0);

    if (strcmp(opcode,value) != 0) {
        return -1;
    }

    return 0;
}

/*
    Args:  
        socket: canal de communication de l'esclave vers le maitre
        type: represente le type de la requete, 4 types possibles (Lock lecture ou ecriture, Unlock lock ou ecriture)
        _offset: decalage par rapport au debut de la memoire pour savoir quel page bloque ou debloque
        _size: taille de la section pour savoir combien de page a bloque ou debloque 
    
    Valeur de retour:
        -1 en cas d'erreur, 1 en cas de reussite
    
    Cette fonction permet d'envoyer les requete de LOCK et UNLOCK au maitre (a l'exception du UNLOCK en ecriture
    car c'est une requete special avec un protocole different)
*/
int8_t sendTo(int32_t socket, char* type, void* _offset, size_t _size){//reussi:1 echec: -1
#ifndef ARCHI
    uint64_t offset = (uint64_t) _offset;
    uint64_t size = (uint64_t) _size;
#endif
#ifdef ARCHI
#if ARCHI == 32
    uint32_t offset = (uint32_t) _offset;
    uint32_t size = (uint32_t) _size;
#else
    uint64_t offset = (uint64_t) _offset;
    uint64_t size = (uint64_t) _size;
#endif
#endif
    char buffer[MESSLEN+2*sizeof(offset)];//on envoie 3 info à la fois
    memset(buffer,0,sizeof(buffer));

    //endianless
    if(sizeof(offset) == 8){
        offset = htobe64(offset);
        size = htobe64(size);
    }else{
        offset = htobe32(offset);
        size = htobe32(size);
    }
        
    memcpy(buffer,type,MESSLEN);
    memcpy(buffer+MESSLEN,&offset,sizeof(offset));
    memcpy(buffer+MESSLEN+sizeof(offset),&size,sizeof(size));

    if(send(socket, buffer, sizeof(buffer), 0) == -1) {
        perror("send");
        return -1;
    }   
    return 1;
}

/*
    Args:
        socket: canal de communication de l'esclave vers le maitre
        offset: decalage par rapport au debut de la memoire pour savoir quelles pages debloque
        size: taille pour savoir combien de page a debloque 
        ptr: pointeur vers le debut de la memoire partage
    
    Valeur de retour:
        -1 en cas d'erreur, 0 en cas de reussite

    Fonction d'envoie de la requete de UNLOCK en ecriture au maitre en respectant 
    le protocole etablit
*/
int8_t send_update(int32_t socket, uint64_t offset, size_t size, void *ptr) {

    // Nombre de page + 1 (forcement au moins 1 page au moins a envoyer)

    #ifndef ARCHI
        uint64_t nbr_pages = 1 + (size/PAGE_SIZE);
    #endif
    #ifdef ARCHI
    #if ARCHI == 32
        uint32_t nbr_pages = 1 + (size/PAGE_SIZE);
    #else
        uint64_t nbr_pages = 1 + (size/PAGE_SIZE);
    #endif
    #endif

    if (sizeof(offset) == 8) {
        offset = htobe64(offset);
        nbr_pages = htobe64(nbr_pages);
    } else {
        offset = htobe32(offset);
        nbr_pages = htobe32(nbr_pages);
    }

    char buf[MESSLEN + sizeof(nbr_pages) + sizeof(offset)];
    memcpy(buf, UNLOW, MESSLEN);
    memcpy(buf+MESSLEN, &nbr_pages, sizeof(nbr_pages));
    memcpy(buf+MESSLEN+sizeof(nbr_pages), &offset, sizeof(offset));

    int nbr_send;

    // Nbr page a envoyer + offset de depart
    if ((nbr_send = send(socket, buf, sizeof(buf), 0)) < 0) {
        perror("send nbr_page");
        return -1;
    }

    if (sizeof(offset) == 8) {
        offset = be64toh(offset);
        nbr_pages = be64toh(nbr_pages);
    } else {
        offset = be32toh(offset);
        nbr_pages = be32toh(nbr_pages);
    }
    
    uint64_t page_position = offset/PAGE_SIZE;
    char opcode[MESSLEN];
    memcpy(opcode, PAGES, MESSLEN);

    for (uint32_t i = 0; i < nbr_pages; i++) {

        unsigned long long addr = (unsigned long long) ptr+((page_position + i)*PAGE_SIZE);
        void *target = (void*) addr;

        if (send(socket, opcode, sizeof(opcode), 0) < 0) {
            perror("send opcode");
            return -1;
        }

        if (send(socket, target, PAGE_SIZE, 0) < 0) {
            perror("send page %d" + i);
        }
    }

    return 0;
}

/*
    Args :
        offset: decalage par rapport au debut de la memoire pour savoir quelles pages debloque
        ptr: pointeur vers le debut de la memoire partage
        size: taille pour savoir combien de page a debloque 
        slv: le pointeur de l'esclave concerne par l'operation
        type: l'operation souhaite 4 possibles, (lock en écriture ou en lecture et les 2 unlock)

    Valeur de retour:
        -1 en cas d'erreur, 1 si réussi

    Fonction d'aiguillage vers la fonction de lock ou unlock souhaité
*/
int8_t send_lock_or_unlock(void *offset, void *ptr, size_t size, struct dsm_slave *slv, char *type){

    char msg_verif[MESSLEN];

    if (strcmp(LOCKR, type) == 0 || strcmp(LOCKW, type) == 0) {
        memcpy(msg_verif, OKLOC, sizeof(msg_verif));
    } else {
        memcpy(msg_verif, OKUNL, sizeof(msg_verif));
    }

    if (strcmp(UNLOW, type) == 0) {
        if (send_update(slv->co.socket, (uint64_t) offset, size, ptr) == -1)
            return -1;
    } else {
        if(sendTo(slv->co.socket, type,offset,size) == -1)
            return -1;
    }

    return verif(msg_verif, slv);
}