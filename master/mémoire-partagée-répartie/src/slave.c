#include "slave.h"
#include "lock_demand.h"

static struct dsm_slave slave;
static uint8_t is_slave_setup = 0;
static uint32_t fault_counter = 0;
static uint8_t answer_lock = 0;


/***********************/
/* PAGEFAULT HANDLING */
/**********************/

/**
 * invalide une page en remettant une protection
 * OU
 * retirer une protection
 * 
 * prot: 0 ou PROT_READ et ou PROT_WRITE
 * addr: position dans la memoire
 * size: taille qui doit etre arrondi à la page pres
*/
//cette fonction a ete conservee car utilisee avant avec les protections en ecriture de uffd...
int alter_protect_page(int prot, void *addr, unsigned long size) {
	if(mprotect(addr, size, prot) < 0)
		return -1;
	else
		return 0;
}


/**
 * initialise userfaultfd avec les informations de l'esclave
 * retourne -1 en cas d'erreur ou uffd si correct
*/
static int init_uffd(struct dsm_slave *slv)
{
	if(slv == NULL)
		return -1;
	
    struct uffdio_api uf_api;
    struct uffdio_register uf_reg;
    void *addr = slv->ptr;
    unsigned long long size = slv->size;
    int32_t uf_fd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    slv->uf_fd = uf_fd;

	memset(&uf_api, 0, sizeof(uf_api));
	memset(&uf_reg, 0, sizeof(uf_reg));

    if (uf_fd < 0) {
		perror("userfaultfd");
		return -1;
	}

    uf_api.api = UFFD_API;
	uf_api.features = 0;
	if (ioctl(uf_fd, UFFDIO_API, &uf_api) < 0) {
		perror("ioctl api");
		return -1;
	}

    uf_reg.range.start = (unsigned long long)addr;
	uf_reg.range.len = size;
	uf_reg.mode = UFFDIO_REGISTER_MODE_MISSING;
	if (ioctl(uf_fd, UFFDIO_REGISTER, &uf_reg) < 0) {
		perror("ioctl register");
		return -1;
	}

	return uf_fd;
}


/**
 * supprime la référence à userfaultfd, à partir des informations de l'esclave
 * retourne -1 en cas d'erreur ou 0 si tout est correct
*/
static int remove_uffd(struct dsm_slave *slv)
{
	if(slv == NULL)
		return -1;
	
    struct uffdio_register uf_reg;

	memset(&uf_reg, 0, sizeof(uf_reg));

    uf_reg.range.start = (unsigned long long)slv->ptr;
    uf_reg.range.len = slv->size;

    if(ioctl(slv->uf_fd, UFFDIO_UNREGISTER, &uf_reg.range) < 0) {
        perror("ioctl unregister");
        return -1;
    }

    slv->uf_fd = -1;
	slv->run = 0;
    return 0;
}


/**
 * mise à jour de la memoire
 * pagefault_addr est aligne sur une bordure de page, page est un buffer de la taille d'une page (mmap ou malloc),  // first_call
 * // est un booleen utilise pour savoir si on est passe par userfaultfd pour executer cette fonction
 * return -1 si erreur ou si on doit arreter le thread, ou 0 sinon
*/
static int update_memory(unsigned long pagefault_addr, struct dsm_slave *slv) {
	if(slv == NULL)
		return -1;
	if(slv->page == NULL || slv->ptr == NULL || slv->run == 0 || slv->uf_fd < 0) {
		slv->run = 0;
		return -1;
	}

	void *page = slave.page;
	struct uffdio_copy uf_cpy;
	uint32_t max_buffer_size = sizeof(slv->size) + PAGE_SIZE; // size : deppend de la config et du protocol
	uint8_t buff[max_buffer_size];						    // max_buffer_size --> plus grande requete ici c'est celle d'opcode PAGES
	int tmp = 0;
	int ret = 0;
	char opcode[MESSLEN]; //opcode de la requete

	/* les champs de tailles selon la config seront extraits et stockee dans tmp */
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

	memset(&uf_cpy, 0, sizeof(uf_cpy));
    
	/* preparer la requete GETPG */
	memcpy(buff, GETPG, MESSLEN); /* opcode */
	offset = pagefault_addr - ((unsigned long long) slave.ptr); /* offset */

	if(sizeof(offset) == 8)
		offset = htobe64(offset);
	else
		offset = htobe32(offset);
	memcpy(buff+MESSLEN, &offset, sizeof(offset));
	
	/* envoyer et recevoir */
	pthread_mutex_lock(&(slv->mutex));
	tmp = send(slv->co.socket, buff, (MESSLEN + sizeof(offset)), 0);

	if(tmp < 0) {
		perror("error on recv() - update_memory()");
		ret = -1;
		slv->run = 0;
        goto cleanup_update_memory;
	}
	if(tmp == 0) {
		fprintf(stderr, "the connection is detected as closed - update_memory()\n");
		ret = -1;
		slv->run = 0;
		goto cleanup_update_memory;
	}

	/* recup la requete et la traiter tant qu'on a pas de réponse d'opcode PAGES (ou une erreur...)
	   si on recois les autres opcode INVAL, ERROR ou RELEA on les traitera */
	do {
        if (recv(slv->co.socket, opcode, MESSLEN, 0) < 0) {
            perror("recv");
        }

		if(strcmp(opcode, RELEA) == 0 || strcmp(opcode, ERROR) == 0) {
			ret = -1; /* forcera a arretre le thread et donc mettre run a 0 */
			goto cleanup_update_memory;
		}
		else if(strcmp(opcode, INVAL) == 0) {

            if (recv(slv->co.socket, buff, sizeof(offset) + sizeof(nb_page), 0) < 0) {
                perror("recv inval mem INVAL");
            }

			/* offset et nb_page sont configure */
			extract_integer(offset, 0, buff);
			extract_integer(nb_page, sizeof(nb_page), buff);

			/* remettre sous endian de l'host */
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
                ret = -1;
                slv->run = 0;
            }

            memcpy(opcode, OKINV, MESSLEN);
            if (send(slv->co.socket, opcode, MESSLEN, 0) < 0) {
                perror("send OKINV");
            }
		}
		else if(strcmp(opcode, PAGES) == 0) { /* ce qu'on cherche en priorite */

			//extract_integer(offset, 0, buff); /* recuperer l'offset et repasser sous endian host */
            
			if(sizeof(offset) == 8)
				offset = be64toh(offset);
			else
				offset = be32toh(offset);
			
			/* verifier que l'offset correspond a l'offset du pagefault, sinon il y a un probleme... */
			if((unsigned long)slv->ptr + offset != pagefault_addr) {
				fprintf(stderr, "error on update_memory(), ptr + offset and pagefault adress are different...\n");
                printf("offset = %ld\n", offset);
                printf("ptr + offset = %p, pagefault addr = %p\n", (void*) ((unsigned long)slv->ptr + offset), (void*) pagefault_addr);
				ret = -1;
				goto cleanup_update_memory;
			}

            if (recv(slv->co.socket, page, PAGE_SIZE, 0) < 0) {
                perror("recv");
            }
            
            /* ----- resoudre le conflit de la page ----- */

            /* ecrire sur la page */
            // Ici on a besoin de savoir si c'est en lecture ou en ecriture (faut pas forcement donner les 2 mais bon)
            //retirer la protection -> page a jour
            if(alter_protect_page(PROT_READ | PROT_WRITE, (void*) (pagefault_addr), PAGE_SIZE) < 0) {
                fprintf(stderr, "alter_protect error\n");
                ret = -1;
                slv->run = 0;
            }

            // On passe toujours dans le first call dans le doute et si l'erreur qui indique qu'on l'a
            // deja fait EEXIST (File exist) se declenche alors on fait un simple memcpy

            uf_cpy.dst = (unsigned long long)pagefault_addr;
            uf_cpy.src = (unsigned long long)page;
            uf_cpy.len = PAGE_SIZE; /* taille d'une page pour ce systeme */
            uf_cpy.mode = 0;
            uf_cpy.copy = 0;

            /* ecrire sur la page */
            if(ioctl(slv->uf_fd, UFFDIO_COPY, &uf_cpy) < 0) {
                if (errno == EEXIST) {
                    memcpy((void*)(pagefault_addr), page, PAGE_SIZE);
                    goto cleanup_update_memory;
                }
                perror("ioctl() page copy");
                ret = -1;
                slv->run = 0;
                goto cleanup_update_memory;
            }
		} else if (strcmp(opcode, PAGES) != 0){
			/* pas des cas qu'on souhaite gerer */
			fprintf(stderr, "unexpected opcode in slave");
			ret = -1;
			goto cleanup_update_memory;
		}

	} while(strcmp(opcode, PAGES) != 0);
	
	cleanup_update_memory:
	pthread_mutex_unlock(&(slv->mutex));
	return ret;
}


/**
 * invalide l'espace memoire
 * return -1 si error ou qu'il faut fermer le thread, sinon 0 (continue)
*/
static int inval_memory(struct dsm_slave *slv) {
	if(slv == NULL)
		return -1;
	if(slv->ptr == NULL || slv->run == 0 || slv->uf_fd < 0) {
		slv->run = 0;
		return -1;
	}
	
	uint32_t max_buffer_size = sizeof(slv->size) + PAGE_SIZE; // size : deppend de la config et du protocol
	uint8_t buff[max_buffer_size];						    // max_buffer_size --> plus grande requete ici c'est celle d'opcode PAGES
	int tmp = 1;
	int ret = 0;
	char opcode[MESSLEN]; //opcode de la requete

	/* les champs de tailles selon la config seront extraits et stockee dans tmp */
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

    memset(opcode, 0, MESSLEN);
   
	pthread_mutex_lock(&(slv->mutex));
    
    // Faudrait certainement que j'utilise ces fonctions de request mais ca me confuse plus qu'autre chose a la fin je m'arrangerai pour les utiliser
    
    if (recv(slv->co.socket, opcode, MESSLEN, 0) < 0) {
        perror("recv");
    }

    //tmp = rec_request_userfaultfd(buff, opcode, slv->co.socket, sizeof(tmp));
	/* verifier les codes de retour */
	if(tmp == -2) {
		ret = 0; //on oublie cette requete et on continue
		goto cleanup_inval_memory;
	}
	if(tmp == -1 || tmp == 0) {
		ret = -1;
		slv->run = 0;
		goto cleanup_inval_memory;
	}

	/* verifier l'opcode */
	if(strcmp(opcode, RELEA) == 0 ||  strcmp(opcode, ERROR) == 0) {
		ret = -1;
		slv->run = 0;
	} else if(strcmp(opcode, INVAL) == 0) {
		/* dependant de la config, on adapte la conversion des entiers sur le reseau (les envoies en big endian) */

        if (recv(slv->co.socket, buff, sizeof(offset) + sizeof(nb_page), 0) < 0) {
            perror("recv inval mem INVAL");
        }

        memcpy(&offset, buff, sizeof(offset));
        memcpy(&nb_page, buff+sizeof(offset), sizeof(nb_page));

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
			ret = -1;
			slv->run = 0;
		}

        memcpy(opcode, OKINV, MESSLEN);
        if (send(slv->co.socket, opcode, MESSLEN, 0) < 0) {
            perror("send OKINV");
        }
	} else {
		/* correspondrait a recevoir une requete PAGES, mais celle ci est provoqué par la gestion des pagefault,
		   on peut donc se dire qu'il y a un pb. De plus, si c'est bien une responce à une requete initié par
		   la gestion des pages, elle ne devrait pas être traitée ici et cela serait une erreur dans le code... */
		fprintf(stderr, "unexpected opcode in slave = %s\n", opcode);
		slv->run = 0;
		ret = -1;
	}

	cleanup_inval_memory:
	pthread_mutex_unlock(&(slv->mutex));
	return ret;
}

//recuperer le SIGSEGV, verifier ou il a lieu, mettre a jour la memoire et la protection
//a la moindre erreur, on termine le processus
static void pf_handler(int sig, siginfo_t *si, void *ctx) {
	unsigned long addr;

    //warning du makefile
    sig = sig;
    ctx = ctx;

	if(slave.ptr == NULL || slave.page == NULL || slave.uf_fd < 0) {
		fprintf(stderr, "null ptr or uf_fd < 0 in SIGSEGV handler\n");
		exit(1);
	}

	//adresse de la faute
	addr = ((long)si->si_addr >> (long)PAGE_SHIFT) << (long)PAGE_SHIFT;

	if((unsigned long)slave.ptr <= addr && (unsigned long)slave.ptr+slave.size >= addr) {
		if(update_memory(addr, &slave) < 0) {
			perror("SIGSEGV - update_memory");
			exit(1);
		}
	}

}

/**
 * fonction executee par un thread pour la gestion des pages de l'esclave.
 * l'argument fournit doit etre une structure dsm_slave correspondant a l'esclave
*/
static void *perform(void *arg)
{
	if(arg == NULL) {
		fprintf(stderr, "error on perform(), arg is NULL\n");
		exit(1); //on ne peut pas retourner de code d'erreur, et si arg est à NULL alors on ne peut pas faire slv->run = 0...
	}

	struct uffd_msg msg;
	struct uffdio_copy uf_cpy;
    struct dsm_slave *slv = (struct dsm_slave *)arg;
	unsigned long long pagefault_addr = 0;
	int32_t nb_read = 0;
	uint32_t uf_fd = slv->uf_fd;

	memset(&uf_cpy, 0, sizeof(uf_cpy));

	while(slv->run) {
		struct pollfd p[2];
		int32_t nb_ready;
		p[0].fd = slv->co.socket;
		p[0].events = POLLIN;
		p[1].fd = uf_fd;
		p[1].events = POLLIN;

		nb_ready = poll(p, 2, 5000); /* timeout utilisee pour verif run == 0*/
		if (nb_ready < 0) {
			perror("poll");
			goto cleanup_perform;
		}

		if(nb_ready == 0) /* retry run condition */
			continue;
        
        // Methode bancal de permettre au lock et unlock de fonctionner independemment du 
        // poll, ici le mutex ne suffirait pas d'apres mes calculs

        pthread_mutex_lock(&(slv->mutex));
        if (answer_lock > 0) {
            answer_lock--;
            pthread_mutex_unlock(&(slv->mutex));
            continue;
        }
        pthread_mutex_unlock(&(slv->mutex));

		/*------------------------------------------------*/
		/* socket */
		if(p[0].revents != POLLIN && p[1].revents != POLLIN) {
			fprintf(stderr, "unexpected revent on socket - poll(), socket, nbr = %d\n", p[0].revents);
			goto cleanup_perform;
		} else if (p[0].revents == POLLIN) { /* initialement, gestion de INVAL, RELEA, ERROR */
			if(inval_memory(slv) < 0)
				goto cleanup_perform;
            continue;
		}
        
		/*------------------------------------------------*/
		/* userfaultfd */
		/* POLLIN recu */
        
        nb_read = read(uf_fd, &msg, sizeof(msg));
        if (nb_read == 0) {
            fprintf(stderr, "EOF read() - userfaultfd\n");
            goto cleanup_perform;
        }
        if (nb_read < 0) {
            perror("read() - userfaultfd");
            goto cleanup_perform;
        }

        //preparation de l'offset du pagefault sur une bordure de page
        pagefault_addr = msg.arg.pagefault.address;
        pagefault_addr = (unsigned long long)((pagefault_addr+(PAGE_SIZE-1)) / PAGE_SIZE) * PAGE_SIZE; //bordure de page

        /* ----- verifier si l'evenement est un pagefault ----- */
        
        if(msg.event == UFFD_EVENT_PAGEFAULT && msg.arg.pagefault.flags == 0) {
            ++fault_counter;
            if(update_memory(pagefault_addr, slv) < 0)
                goto cleanup_perform;
        }
        /* ne devrait pas arriver - cas qu'on ne cherche pas a gerer */
        
        else {
            fprintf(stderr, "unexpected event on userfaultfd %llu\n", msg.arg.pagefault.flags);
        }
            
		 /* POLLIN case */
	} /* while */

    cleanup_perform:
	slv->run = 0;
    if (close(slv->co.socket)) {
        perror("close");
    }
	pthread_exit(0);
}


/**************************/
/* END PAGEFAULT HANDLING */
/**************************/


/* 
    Valeur de retour:
        Retourne 0 si correctement detruit, -1 sinon

    Detruit notre esclave et desalloue ces ressources locales, previent 
    egalement le maitre de sa deconnexion 
*/
int destroy_slave() {

    pthread_mutex_lock(&(slave.mutex));
    answer_lock++;

    char msg[MESSLEN];
    memcpy(msg, "DECO?\0", MESSLEN);

    if (write(slave.co.socket, msg, sizeof(msg)) < 0) {
        perror("write error"); 
        goto cleanup_destroy_slave;
    }

    if (read(slave.co.socket, msg, sizeof(msg) < 0)) {
        perror("read error");
        goto cleanup_destroy_slave;
    }

    if (remove_uffd(&slave) == -1) {
        fprintf(stderr, "error remove_uffd\n");
    }

    if (shutdown(slave.co.socket, SHUT_RDWR) < 0) {
        perror("shutdown");
    }
    
    if (close(slave.co.socket) < 0) {
        perror("close");
    }

    if (munmap(slave.ptr, slave.size) == -1) {
        perror("mumap");
        exit(4);
    }

    pthread_kill(slave.th, SIGSTOP); 
    pthread_mutex_unlock(&(slave.mutex));

    printf("END SLAVE\n");

    if (strcmp(msg, "DECO!\0") == 0) {
        return 0;
    } 

    cleanup_destroy_slave:
    pthread_mutex_unlock(&(slave.mutex));
    return -1;
}

/*
    Args:
        host_name: nom de domaine du maitre pour pouvoir l'atteindre
        port: le numero de port a utiliser

    Valeur de retour:
        Renvoie un pointeur sur le debut du segment de memoire partage

    Initialise notre esclave et donc sa connexion avec le maitre, cree le thread
    qui va s'occuper d'ecouter les messages du maitre et de faire le traitement 
    reel de l'esclave (perform)
*/
void* init_slave(char* host_name, uint16_t port) {
	if(is_slave_setup)
		return NULL;

    struct addrinfo* result;
    struct sockaddr_in adress;
    void* ptr;
    int32_t sock;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	struct sigaction sa;

    //////////// Creation de la socket de connexion au maitre + connect ////////////

    if ((sock = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("socket");
        exit(1);
    }

    if (getaddrinfo(host_name, NULL, NULL, &result) != 0) {
        perror("getaddrinfo"); 
        exit(1);
    }

    adress.sin_addr = ((struct sockaddr_in*)result->ai_addr)->sin_addr;
    adress.sin_family = AF_INET;
    adress.sin_port = htons(port);  
    
    if (connect(sock, (struct sockaddr *) &adress, sizeof(adress)) < 0) {
        perror("connect"); 
        exit(1);
    }

    ////////////  Fin creation de la socket qui le relie au maitre + connect ////////////  

    //////////// Lecture size de la memoire du maitre + mmap correspondant ////////////  

    struct size_msg msg;
    char type_msg[MESSLEN];
    memcpy(type_msg, "JOIN?\0", MESSLEN);

    if (write(sock, type_msg, sizeof(type_msg)) < 0) {
        perror("write error"); 
        exit(1);
    }

    //////////// Lecture size de la memoire du maitre + mmap correspondant ////////////  

    if (read(sock, &msg, sizeof(msg)) < 0) {
        perror("read error"); 
        exit(1);
    }
    
    if (strcmp(msg.msg, "ERROR\0") == 0) {
        perror("type of message receive : error");
        exit(1);
    } else if (strcmp(msg.msg, "JOIN!\0") != 0) {
        perror("type of message unknown");
        exit(1);
    }

    msg.page_size = be32toh(msg.page_size);
    if (sizeof(msg.size) == 8) {
        msg.size = be64toh(msg.size);
    } else {
        msg.size = be32toh(msg.size);
    }

    // Taille de page differente entre le maitre et l'esclave pour l'instnt on ne souhaite pas gerer ce genre de cas
    if (msg.page_size != PAGE_SIZE) {
        fprintf(stderr, "Different page size with the master\n");
        exit(1);
    }

    // Archi differentes (64 ou 32 bits)
    if (msg.archi != sizeof(msg.size)) {
        fprintf(stderr, "Different archi with the master 64/32 bits\n");
        exit(1);
    }

    // un esclave n'a aucune valeur a son init 
    // tant qu'il ne cherche pas a lire ou a ecrire c'est un magnifique exemple de coup du chapeau
    
    if ((ptr = mmap(NULL, msg.size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

	slave.page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (slave.page == MAP_FAILED) {
		perror("mmap() - page buffer");
		munmap(ptr, msg.size);
		exit(1);
	}

    //////////// Fin Lecture size de la memoire du maitre + mmap correspondant ////////////  


    //////////// Init struct slave ////////////  

    struct connection connection;
    connection.adr = adress;
    connection.socket = sock;

    slave.co = connection;    
    slave.ptr = ptr;
    slave.mutex = mutex;
    slave.size = msg.size;
    slave.run = 1;
    
    
    if((slave.uf_fd = init_uffd(&slave)) < 0) {
        perror("init uffd");
        exit(1);
    }
    
    // preparation du thread
    if(pthread_create(&slave.th, NULL, perform, &slave) < 0) {
        perror("pthread_create");
        remove_uffd(&slave);
        exit(1);
    }

    //////////// Fin init struct slave ////////////  

    //////////// Envoie sock_addr au maitre ////////////

    // Quasi sur qu'on en a pas besoin mais bon pour l'instant je laisse

    if (write(sock, &(adress), sizeof(adress))  < 0) {
        perror("write error");
        remove_uffd(&slave);
        exit(1);
    }

	//handler SIGSEGV
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = pf_handler;
    sa.sa_flags = SA_SIGINFO;
	if(sigaction(SIGSEGV, &sa, NULL) < 0) {
		perror("sigaction - init slave");
		exit(1);
	}

    //////////// Fin envoie ////////////
	is_slave_setup = 1;
    return ptr;
}


/*
    Args:
        adr: pointeur de depart de la memoire partage
        size: taille que l'on veut lock ou unlock
        type: l'operation qu'on tente de faire il en y a 4 de disponible, lock en ecriture ou en lecture et les unlock reciproque

    Valeur de retour:
        Soit -1 en cas d'erreur, soit 1 en cas de reussite
    
    Fonction d'aiguillage qui permet de voir quelle operation de lock ou unlock effectue puis de l'envoyer 
    sur la fonction envoyant la requete adequate au maitre
*/
int8_t lock(void* adr, size_t size, char *type) {
    pthread_mutex_lock(&(slave.mutex));
    answer_lock++;
    void* offset = (void*) ((ulong)((char*)adr) - (ulong)((char*)slave.ptr));

    // Verification sur la size pour pas qu'elle depasse notre size de memoire partagee
    if ((((size_t) (offset)) + size) > slave.size) {
        size = slave.size;
    }

    if(send_lock_or_unlock(offset, slave.ptr, size, &slave, type) == -1) {
        pthread_mutex_unlock(&(slave.mutex));
        fprintf(stderr, "send_lock_or_unlock failure\n");
        return -1;
    }
    pthread_mutex_unlock(&(slave.mutex));
    return 1;
}