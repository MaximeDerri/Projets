#include "master.h"
#include "lock_demand.h"
#define NB_REQ_PENDANT 5

static struct dsm_master master;

pthread_t listen_thread;

// Handler de signal pour la destruction du maitre 
void sig_handler_loop(int signum)
{
    signum = signum; //warning du makefile
    destroy_master();
    exit(0);
}

// Handler de signal pour tuer le thread d'ecoute des esclaves
void sig_handler_kill_thread(int signum) 
{
    signum = signum;  //warning du makefile
    // Desallouer toute les ressources donne pour cette esclave
    pthread_exit(NULL);
}

/*
    Arg:
        size : la taille que l'on souhaite allouer en memoire (arrondis a une page superieur)

    Valeur de retour:
        Retourne NULL en cas d'echec sinon retourne un pointeur qui pointe au debut de cette memoire
        partage

    Cette fonction permet de creer la memoire partage repartis et donc entre outre 
    d'initialiser le canal permettant la communication avec les esclaves.
*/
void *init_master(size_t size) 
{
    struct sockaddr_in adress;
    void *ptr;
    unsigned long page_size = PAGE_SIZE;
    int32_t sock;


    //////////// Creation de la socket ////////////

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket"); 
        return NULL;
    }

    adress.sin_addr.s_addr = htonl(INADDR_ANY);
    adress.sin_port = htons(PORTSERV);
    adress.sin_family = AF_INET;

    // Si l'utilisateur veut augmenter ou baisser la taille du buffer de reception du maitre

#ifdef BUFFER
#if (BUFFER > (page_size * 3))
    if (setsockopt(socket, SOL_SOCKET SO_REUSEADDR,
        &(int){1}, BUFFER)) {
            perror("setsockopt");
            return NULL;
    }
#endif
#endif
    
    //////////// Fin creation de la socket + bind ////////////


    //////////// Creation du segment de memoire partage repartie ////////////

    // size + ajustement pour la prochaine page si besoin
    size_t new_size = (size % PAGE_SIZE == 0) ? size : (PAGE_SIZE - (size % PAGE_SIZE) + size);

    if ((ptr = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
        perror("mmap");
		return NULL;
    }

    //////////// Fin creation du segment de memoire partage repartie ////////////

    
    //////////// Init struct dsm master ////////////

    // Forcement un entier vu qu'on a ajuste au nombre de page superieur
    int nbr_page = new_size / PAGE_SIZE;

    struct connection connection;
    connection.adr = adress;
    connection.socket = sock;

    master.co = connection;

    // Chaque client ajoutera une struct slave, a utilise dans loop_master

    master.slaves = NULL;
    pthread_mutex_init(&master.mutex, NULL);
    master.ptr = ptr;
    master.size = new_size;

    // Init pages mutex

    // arrondis au superieur
    master.pages = malloc(sizeof(struct page) * nbr_page);
    if (master.pages == NULL) {
        perror("malloc");
        return NULL;
    }

    // Initialisation pour lock
    for(int i=0;i<nbr_page;i++){ // initialement les pages ont tout les autorisations
        pthread_mutex_init(&master.pages[i].mutex,NULL);
    }
    //////////// Fin init struct dsm master ////////////

    return ptr;
}

/*
    Args:
        addr: un pointeur sur le debut de la memoire du maitre
        _offset: decalage par rapport au pointeur addr 
        size: la taille a lock ou unlock (nous indique le nombre de page a traite pour cette operation)
        lock: unlock = 0, lock = 1, nous precise l'operation qu'on tente d'operer

    Gestion des lock et unlock des esclaves
*/
int8_t setProtection(void *addr, void *_offset, void *_size, int8_t lock){
#ifndef ARCHI
    uint64_t offset = (uint64_t)_offset;
    uint64_t size = (uint64_t)_size;
#endif
#ifdef ARCHI
    #if ARCHI == 32
    uint32_t offset = (uint32_t)_offset;
	uint32_t size = (uint32_t)_size;
#else
    uint64_t offset = (uint64_t)_offset;
    uint64_t size = (uint64_t)_size;
#endif
#endif
    int page_position = offset/PAGE_SIZE;//calculer ou se trouve la page
    int nbrPageConcerne=(size/PAGE_SIZE)+1;//au moins 1 page

    addr = addr; //warning du makefile

    for(int i = page_position; i < (page_position+nbrPageConcerne); i++){
        if(lock){
            pthread_mutex_lock(&(master.pages[i].mutex));
        }else{
            pthread_mutex_unlock(&(master.pages[i].mutex));
        }
    }
    return 1;
}

/*
    Args:
        args: pointeur qui doit contenir une structure lock_struct

    Fonction auxiliaire de verification de setProtection pour finir par obtenir une 
    gestion de la concurrence sur les esclaves venant utiliser notre memoire et plus 
    precisement sur ceux qui veulent par exemple ecrire sur la meme page
*/
void *lock_and_unlock(void *args){

    struct lock_struct *lock_struct = (struct lock_struct*) args;
#ifndef ARCHI
    uint64_t offset = (uint64_t)lock_struct->_offset;
    uint64_t size = (uint64_t)lock_struct->_size;
#endif
#ifdef ARCHI
    #if ARCHI == 32
    uint32_t offset = (uint32_t)lock_struct->_offset;
	uint32_t size = (uint32_t)lock_struct->_size;
#else
    uint64_t offset = (uint64_t)lock_struct->_offset;
    uint64_t size = (uint64_t)lock_struct->_size;
#endif
#endif

    int8_t rr = setProtection(master.ptr, (void*)offset, (void*)size, lock_struct->operation);
    if(rr == -1){
        send(lock_struct->fd, ERROR, MESSLEN, 0);
    }

    if (lock_struct->operation == LOCK) {
        if (send(lock_struct->fd, OKLOC, MESSLEN, 0) < 0) {
            perror("send");
        }
    } else {
        if (send(lock_struct->fd, OKUNL, MESSLEN, 0) < 0) {
            perror("send");
        }
    }
    return 0;
}

/*
    Fonction du thread qui ecoute les nouvelles connections des esclaves et instancie 
    leur canal de communication avec l'esclave 
*/
void *listen_t(void *arg) 
{
    // handler de signal pour pouvoir l'arreter en cas d'un SIGINT
    struct sigaction sig_act;

    arg = arg; //warning du makefile

    sig_act.sa_handler = sig_handler_kill_thread;
    sigfillset(&sig_act.sa_mask);
    sigdelset(&sig_act.sa_mask, SIGQUIT);
    sig_act.sa_flags = 0;
    
    sigaction(SIGQUIT, &sig_act, NULL);

    int32_t com_sock;
    socklen_t fromlen = sizeof(master.co.adr);
    struct sockaddr_in addr_client;

    uint32_t nbr_page = master.size / PAGE_SIZE;
    uint32_t nbr_page_div_8 = (nbr_page % 8 == 0) ? (nbr_page / 8) : (nbr_page/8 + 1);

    struct size_msg msg;
    msg.page_size = htobe32(PAGE_SIZE);
    msg.archi = sizeof(msg.size);
    char type_msg[6];
    
    if (bind(master.co.socket, (struct sockaddr *)&master.co.adr, sizeof(master.co.adr)) < 0) {
        perror("bind");
        exit(1); 
    }  

    if (listen(master.co.socket, NB_REQ_PENDANT) == -1) {
        perror("listen");
        exit(1);
    }
    
    while (1) {

        /*************** Protocole d'acceptation d'un esclave ***************/

        if ((com_sock = accept(master.co.socket, (struct sockaddr *)&(master.co.adr), &fromlen)) < 0) {
            perror("accept error"); 
        }
    
        if (read(com_sock, type_msg, sizeof(char) * 6) < 0) {
            perror("read error");
        }
        
        if (strcmp(type_msg, "JOIN?\0") == 0) {
            memcpy(msg.msg, "JOIN!\0", 6);
            if (sizeof(master.size) == 8) {
                msg.size = htobe64(master.size);
            } else {
                msg.size = htobe32(master.size);
            }
            if (write(com_sock, &(msg), sizeof(msg)) < 0) {
                perror("write join msg error");
                memcpy(msg.msg, "ERROR\0", 6);
                msg.size = 0;
                if (write(com_sock, &(msg), sizeof(msg)) < 0) {
                    perror("write error msg error");
                }
            }
        }

        if (read(com_sock, &addr_client, sizeof(struct sockaddr_in))  < 0) {
            perror("read error"); 
        }

        /*************** Fin protocole d'acceptation d'un esclave ***************/

        /*************** Creation et ajout de la structure representant le nouvel esclave ***************/
        
        struct slave *tmp_slave = (struct slave*) malloc(sizeof(struct slave));
        if(tmp_slave == NULL) {
            perror("malloc");
            pthread_exit(NULL);
        }
        
        tmp_slave->co.socket = com_sock;
        tmp_slave->co.adr = addr_client;

        pthread_mutex_lock(&(master.mutex));
        tmp_slave->next = master.slaves;
        tmp_slave->prev = NULL;
        master.slaves = tmp_slave;
        if (master.slaves->next != NULL) {
            master.slaves->next->prev = master.slaves;
        }
        pthread_mutex_unlock(&(master.mutex));


        for (uint32_t i = 0; i < nbr_page_div_8; i++) {
            master.slaves->page_map[i] = 0;
        }

        /*************** Fin creation et ajout de la structure representant le nouvel esclave ***************/
    }
}


/* 
    Args: 
    slave_index = Le slave_index est l'indice de l'esclave que nous voulons deconnecter dans notre 
    liste chaine 

    Valeur de retour: 0 en cas de reussite, -1 sinon 

    Description de la fonction:
    Cette fonction nous permet de deconnecter un esclave en deplacant simplement les pointeurs prev et next de la 
    liste. En envoyant un message DECO! pour signaler la deconnexion reussis a l'esclave correspondant.

*/
int deco_slave(struct slave *slv) {

    pthread_mutex_lock(&(master.mutex));

    // Si un esclave se deconnecte de maniere imprevu sans avoir fait ses unlock, il doit tout de meme 
    // les rendre (pour ca il faudrait stocker les pages lock par chaque esclave)

    /*
    int nbr_page = master.size / PAGE_SIZE;

    for(int i = 0; i < nbr_page; i++){
        pthread_mutex_unlock(&(master.pages[i].mutex));
    }  
    */

    char opcode[MESSLEN] = "DECO!\0";

    if (slv == master.slaves) {
        master.slaves = slv->next;
    } else {

        slv->prev->next = slv->next;
        
        if (slv->next != NULL) {
            slv->next->prev = slv->prev;
        }

    }

    if (write(slv->co.socket, opcode, MESSLEN) < 0) {
        perror("write");
        return -1;
    }

    if (close(slv->co.socket) < 0) {
        perror("close");
        return -1;
    }

    free(slv);

    pthread_mutex_unlock(&(master.mutex));

    return 1;
}

/*
    Boucle infini que le maitre devra executer, cette fonction se charge de regarder 
    les differentes requetes des esclaves et de reagir en fonction de leurs code de 
    requete (nomme opcode)
*/
void loop_master() {

    // Handler de signal pour tuer le processus a la fin de maniere propre avec le destroy_master

    struct sigaction sig_act;
    sig_act.sa_handler = sig_handler_loop;
    sigfillset(&sig_act.sa_mask);
    sigdelset(&sig_act.sa_mask, SIGINT);
    sig_act.sa_flags = 0;

    sigaction(SIGINT, &sig_act, NULL);

    // Un thread qui ecoute les nouveaux esclaves entrants
    
    if (pthread_create(&listen_thread, NULL, listen_t, NULL) < 0) {
        perror("pthread");
        exit(3);
    }

#ifndef ARCHI
    uint64_t offset = 0;
    uint64_t size = 0;
#endif
#ifdef ARCHI
#if ARCHI == 32
    uint32_t offset = 0;
	uint32_t size = 0;
#else
	uint64_t offset = 0;
    uint64_t size = 0;
#endif
#endif

    struct pollfd p[MAX_SLAVE+1];
    int nb_ready;
    struct slave *cur_slave;
    // Buffer de l'opcode de la requete qu'on va recevoir
    char opcode[6];

    while (1) {
        // On attend qu'un esclave nous envoit un message

        int i = 0;
        for (cur_slave = master.slaves; cur_slave != NULL; cur_slave = cur_slave->next) {
            p[i].fd = cur_slave->co.socket;
            p[i].events = POLLIN;
            i++;
        }

        if ((nb_ready = poll(p, i, 5000)) == -1) {
            perror("poll");
            exit(1);
        }

        if (nb_ready == 0) {
            continue;
        }

        // On verifie quel esclave a envoye une info a lire
        for (int k = 0; k < i; k++) {
            
            // Cette socket n'a pas recu de message on passe a la suivante
            if (p[k].revents == 0) {
                continue;
            }

            // Cas d'incoherence on attend seulement les evenement POLLIN
            if (p[k].revents != POLLIN) {
                if (p[k].revents & POLLIN) {
                    // Probleme sur la socket mais il y a tout de meme une donnee a lire
                    printf("Une donnee a lire sur la socket, revent = %d\n", p[k].revents);
                } else {
                    // Problem avec cette esclave on le deco
                    struct slave *slv = master.slaves;
                    for (int t = 0; t < k; t++) {
                        slv = slv->next;
                    }
                    deco_slave(slv);
                    continue;
                }
            }

            opcode[0] = '\0';

            int32_t nb_read;
            if ((nb_read = read(p[k].fd, opcode, sizeof(opcode))) < 0) {
                perror("read opcode");
                continue;
            }

            // Si rien lu alors cela veut dire que la connexion est ferme ou a eu un problem on peut donc la fermer
            if (nb_read == 0) {
                // Connection socket ferme 
                printf("deconnexion esclave\n");
                struct slave *slv = master.slaves;
                for (int t = 0; t < k; t++) {
                    slv = slv->next;
                }
                deco_slave(slv);
                break;
            }

            pthread_t treatment_thread;

            
            // Code pour la deconnection d'un esclave
            if (strcmp(opcode, "DECO?") == 0) {
                // Ici on choisit de ne pas creer de thread pour gerer cette requete pour garder une coherence sur les 
                // socket on scrute avec poll (ici l'esclave devrait deja etre deconnecte) donc plus besoin de l'observer
                // et pour cela on laisse la fonction qui l'enleve de la liste s'executer avant de continuer

                struct slave *slv = master.slaves;
                for (int t = 0; t < k; t++) {
                    slv = slv->next;
                }

                deco_slave(slv);

            // Gestion des codes de requetes de lock et unlow sauf le unlock en ecriture traite separement 
            } else if ((strcmp(opcode, LOCKR) == 0) || (strcmp(opcode,LOCKW) == 0) || (strcmp(opcode,UNLOR) == 0)) {
                
                // Recuperation necessaire a la requete et precise dans notre protocole applicatif 

                char buf[sizeof(offset) + sizeof(size) + MESSLEN];

                if (recv(p[k].fd, buf, sizeof(offset) + sizeof(size), 0) < 0) {
                    perror("recv requete LOCK");
                } 

                memcpy(&offset, buf, sizeof(offset));
                memcpy(&size, buf+sizeof(offset), sizeof(size));

                
                // Gestion du big ou little endian 
                if(sizeof(offset) == 8){
                    offset = be64toh(offset);
                    size = be64toh(size);
                }else{
                    offset = be32toh(offset);
                    size = be32toh(size);
                }

                // Creation de la structure lock_struct contenant les informations permettant de realiser les 
                // differents lock et unlock
                struct lock_struct *lock_struct = (struct lock_struct*) malloc(sizeof(struct lock_struct));
                lock_struct->_offset = (void*) offset;
                lock_struct->_size = (void*) size;
                lock_struct->fd = p[k].fd;

                if (strcmp(opcode, LOCKR) == 0) {
                    //lock Read
                    lock_struct->operation = LOCK;
                    pthread_create(&treatment_thread, NULL, lock_and_unlock, (void*) lock_struct);
                }

                if (strcmp(opcode,LOCKW) == 0) {
                    //lock Write
                    lock_struct->operation = LOCK;
                    pthread_create(&treatment_thread, NULL, lock_and_unlock, (void*) lock_struct);
                }

                if (strcmp(opcode,UNLOR) == 0) {
                    //unlock Read
                    lock_struct->operation = UNLOCK;
                    pthread_create(&treatment_thread, NULL, lock_and_unlock, (void*) lock_struct); 
                }
            
            // Gestion du code de requete de unlock en ecriture
            } else if (strcmp(opcode,UNLOW) == 0) {
                // D'abord on recupere et on met a jour les n pages que l'esclave a modifie 
                // puis on envoie les INVAL aux esclaves possedant deja cette page

                #ifndef ARCHI
                    uint64_t nbr_pages;
                #endif
                #ifdef ARCHI
                #if ARCHI == 32
                    uint32_t nbr_pages;
                #else
                    uint64_t nbr_pages;
                #endif
                #endif

                char buf[sizeof(nbr_pages) + sizeof(offset) + MESSLEN];
                
                if (recv(p[k].fd, buf, sizeof(nbr_pages) + sizeof(offset), 0) < 0) {
                    perror("recv requete UNLOW");
                } 

                memcpy(&nbr_pages, buf, sizeof(nbr_pages));
                memcpy(&offset, buf+sizeof(nbr_pages), sizeof(offset));

                if (sizeof(offset) == 8) {
                    offset = be64toh(offset);
                    nbr_pages = be64toh(nbr_pages);
                } else {
                    offset = be32toh(offset);
                    nbr_pages = be32toh(nbr_pages);
                }
    
                // On met a jour la page en question 
                uint32_t page_position = offset/PAGE_SIZE;

                for (uint32_t j = 0; j < nbr_pages; j++) {

                    // Reception requete 
                    if (recv(p[k].fd, opcode, sizeof(opcode), 0) < 0) {
                        perror("recv PAGES UNLOW");
                    } 

                    unsigned long long adr = (unsigned long long) master.ptr+((page_position+j)*PAGE_SIZE);
                    void *target = (void*) adr;

                    int a;

                    if ((a = recv(p[k].fd, target, PAGE_SIZE, 0)) < 0) {
                        perror("recv target UNLOW");
                    }
                }

                // Pour tous les esclaves connecte, on envoit la requete d'invalidation

                memcpy(opcode, INVAL, MESSLEN);

                int j = 0;

                if (sizeof(offset) == 8) {
                    offset = htobe64(offset);
                    nbr_pages = htobe64(nbr_pages);
                } else {
                    offset = htobe32(offset);
                    nbr_pages = htobe32(nbr_pages);
                }

                memcpy(buf, opcode, MESSLEN);
                memcpy(buf+MESSLEN, &offset, sizeof(offset));
                memcpy(buf+MESSLEN+sizeof(offset), &nbr_pages, sizeof(nbr_pages));

                // Envoyer la requete d'INVAL a tous les esclaves 
                for (cur_slave = master.slaves; cur_slave != NULL; cur_slave = cur_slave->next) {
                    if (j != k) {
                        if (send(p[j].fd, buf, sizeof(buf), 0) < 0)
                            perror("send INVAL");
                    }
                    j++;
                }

                if (sizeof(offset) == 8) {
                    offset = be64toh(offset);
                    nbr_pages = be64toh(nbr_pages);
                } else {
                    offset = be32toh(offset);
                    nbr_pages = be32toh(nbr_pages);
                }

                struct lock_struct *lock_struct = (struct lock_struct*) malloc(sizeof(struct lock_struct));
                lock_struct->_offset = (void*) offset;
                lock_struct->_size = (void*) size;
                lock_struct->fd = p[k].fd;

                // unlock write
                lock_struct->operation = UNLOCK;
                pthread_create(&treatment_thread, NULL, lock_and_unlock, (void*) lock_struct);

            // Gestion du code de requete GETPG
            } else if ((strcmp(opcode, GETPG) == 0)) {

                if (read(p[k].fd, &offset, sizeof(offset)) < 0) {
                    perror("read offset");
                }

                if(sizeof(offset) == 8){
                    offset = be64toh(offset);
                } else {
                    offset = be32toh(offset);
                }

                uint32_t page_position = offset/PAGE_SIZE;

                unsigned long long adr = (unsigned long long) master.ptr+(page_position*PAGE_SIZE);
                void *target = (void*) adr;
                memcpy(opcode, PAGES, MESSLEN);
                
                if (send(p[k].fd, opcode, MESSLEN,0) < 0) {
                    perror("send PAGES GETPG");
                }

                int a;
                if ((a = send(p[k].fd, target, PAGE_SIZE,0)) < 0) {
                    perror("send target GETPG");
                }
            }
            
            size = 0;
            offset = 0;
        }
    }
}

/*
    Fonction de destruction du segment de memoire partage repartis, libere toutes les ressources 
    utilise par le maitre et previent les esclaves que celui n'est plus present avec le message 
*/
int destroy_master() 
{
    // Kill thread listen, ca pourrait etre intessant de recuperer sa valeur de retour a l'avenir
    pthread_kill(listen_thread, SIGQUIT);
    pthread_join(listen_thread, NULL);

    char msg[MESSLEN] = RELEA;

    // Send RELEA message a tous les processus esclaves pour mettre fin 
    pthread_mutex_lock(&(master.mutex));

    for (struct slave *cur = master.slaves; cur != NULL; cur = cur->next) {
        if (send(cur->co.socket, msg, MESSLEN, 0) < 0) {
            perror("send");
        }
    }

    pthread_mutex_unlock(&(master.mutex));

    // On detruit le segment de memoire partage
    if (munmap(master.ptr, master.size) == -1) {
        perror("mumap");
        exit(4);
    }

    free(master.pages);

    shutdown(master.co.socket, SHUT_RDWR);
    close(master.co.socket);
    
    // Liberer les ressources des esclaves et fermer les socket encore ouvertes
    struct slave *tmp_slave = master.slaves;
    struct slave *cur_slave = master.slaves;

    while (cur_slave != NULL) {
        tmp_slave = cur_slave;
        cur_slave = cur_slave->next;
        shutdown(tmp_slave->co.socket, SHUT_RDWR);
        close(tmp_slave->co.socket);
        free(tmp_slave);
    }

    printf("END MASTER\n");

    return 0;
}