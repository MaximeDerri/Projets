#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define INIT 0
#define SRCH 1
#define NOTIF_KEY 2
#define FINISH 3

// Etat possible de l'algorithme Hirschberg & Sinclair
#define NSP 0
#define BATTU 1
#define ELU 2

// Type de message possible Hirschberg & Sinclair
#define IN 0
#define OUT 1
#define TERM 2

#define NR_NODE 7
#define NR_SIMU (NR_NODE+1)

#define NR_KEY 64 //2^n

/*
    Le buffer de reception qui doit contenir au maximum soit les 3 infos pour les messages OUT :
    emetteur, initiateur, distance
    Soit pour les messages TERM:
    NR_NODE pour la taille de la liste des id = NR_NODE
    + 1 pour l'indice de la liste
    + 1 pour l'id leader
    = NR_NODE + 1 + 1 

    il existe un cas degenere ou le message OUT contient plus de donnee que le message TERM quand 
    NR_NODE = 0, c'est la raison pour laquelle nous choisissons pour la securite de mettre un 3 + NR_NODE (et non un 2)
*/
#define BUFFER_SIZE (3 + NR_NODE)

static int bits_of_keys = (int)log2l(NR_KEY);

//hash function
static int hash(int val) {
    val %= NR_KEY;
    return ((int)powl(val,3) ^ val) % NR_KEY;
}

//finger table
static int compute_finger(int id, int i) {
    return (id + (int)(powl(2,i))) % NR_KEY;
}

//fill res with the MPI id and the node id related to val which is a key
static void find_id_finger(int id[], int len, int val, int res[]) {
    int i;
    for (i = 1; i < len; ++i) {
        if (val > id[i-1] && val <= id[i]) {
            res[0] = i;
            res[1] = id[i];
            return;
        }
    }

    //greater than the max node id ==> id[1]] (0 ~> 1)
    res[0] = 1;
    res[1] = id[1];
}


/*
    Changement d'etape dans l'algorithme, on passe a l'etape suivante de 
    l'algorithme en envoyant un message au voisin droit et gauche 

    k : numero de l'etape en cours pour ce processus 
    nb_in : nombre de message in recu (ici reinitialise a 0)
    id : identite du processus courant
    VD : voisin droit 
    VG : voisin gauche
*/
void initier_etape_suivante(int k, int *nb_in, int id, int VD, int VG) {

    *nb_in = 0;

    // Indice 0 = emetteur, indice 1 = initiateur, indice 2 = distance, indice 3 = indice du tableau, indice 4 jusqu'a la fin contenu du tableau des id des procs
    int buffer[BUFFER_SIZE];
    buffer[0] = id;
    buffer[1] = id;
    buffer[2] = pow(2, (double) k);
    // Indice de la liste
    buffer[3] = 0;
    // Tab des id procs
    memset(&(buffer[4]), 0, NR_NODE);

    // Envoie au voisin droit 
    MPI_Send(buffer, BUFFER_SIZE, MPI_INT, VD, OUT, MPI_COMM_WORLD);

    // Envoie au voisin gauche
    MPI_Send(buffer, BUFFER_SIZE, MPI_INT, VG, OUT, MPI_COMM_WORLD);
}

/*
    Recevoir un message OUT

    Ici le buffer contient 3 donnee :
    indice 0 : emetteur, 
    indice 1 : initiateur, 
    indice 2 : distance,

    etat : etat du proccesus d'election soit BATTU, ELU ou NSP (Ne Sait Pas)
    id : identite du processus courant
    VD : voisin droit
    VG : voisin gauche
    is_initiateur : booleen representant si ce processus est un initiateur
*/
void recevoir_out(int buffer[], int *etat, int id, int VD, int VG, int is_initiateur) {
    
    int emetteur = buffer[0];
    int initiateur = buffer[1];
    int distance = buffer[2];

    if (is_initiateur == 0 || id < initiateur) {
        *etat = BATTU;
        if (distance > 1) {
            // On decremente la distance que ce message a a parcourir
            buffer[2]--;
            if (emetteur == VD) {
                // Notre emetteur devient ce processus donc son id
                buffer[0] = id;
                MPI_Send(buffer, 3, MPI_INT, VG, OUT, MPI_COMM_WORLD);
            } else {
                // Notre emetteur devient ce processus donc son id
                buffer[0] = id;
                MPI_Send(buffer, 3, MPI_INT, VD, OUT, MPI_COMM_WORLD);
            }
        } else {
            // Si la distance passe a 0 on change de sens et on envoit un message IN pour 
            // retourner jusqu'a l'initiateur
            if (emetteur == VD) {
                buffer[0] = id;
                MPI_Send(buffer, 2, MPI_INT, VD, IN, MPI_COMM_WORLD);
            } else {
                buffer[0] = id;
                MPI_Send(buffer, 2, MPI_INT, VG, IN, MPI_COMM_WORLD);
            }
        }
    // Si le message OUT a fait tout le tour de notre anneau et revient a son initiateur alors on sait qu'il
    // a BATTU tous ses concurrents et qu'il peut donc etre designe leader 
    } else if (initiateur == id) {
        *etat = ELU;

        // La pour reduire la taille des messages de terminaison on envoit juste le num du leader
        buffer[0] = id;
        buffer[1] = 0;

        // Il finit par envoyer un message a son voisin de gauche qui fera finalement le tour de l'anneau
        // pour annoncer que c'est lui qui est devenu le leader
        MPI_Send(buffer, BUFFER_SIZE, MPI_INT, VG, TERM, MPI_COMM_WORLD);
    }
}

/*
    Recevoir un message IN et le traiter

    Ici le buffer contient :
    indice 0 : emetteur 
    indice 1 : initiateur

    etat : etat du proccesus d'election soit BATTU, ELU ou NSP (Ne Sait Pas)
    id : identite du processus courant
    VD : voisin droit
    VG : voisin gauche
    nb_in : nombre de message in recu
    k : numero de l'etape en cours pour ce processus 
*/
void recevoir_in(int buffer[], int *etat, int id, int VD, int VG, int *nb_in, int *k) {
    
    int emetteur = buffer[0];
    int initiateur = buffer[1];

    if (id == initiateur) {
        (*nb_in)++;
        // Si les 2 messages IN recu on peut passer a l'etape suivante 
        if ((*nb_in) == 2) {
            (*k)++;
            initier_etape_suivante(*k, nb_in, id, VD, VG);
        }
    } else {
        // On redirige le message jusqu'a arriver a l'initiateur
        if (emetteur == VD) {
            buffer[0] = id;
            MPI_Send(buffer, 2, MPI_INT, VG, IN, MPI_COMM_WORLD);
        } else {
            buffer[0] = id;
            MPI_Send(buffer, 2, MPI_INT, VD, IN, MPI_COMM_WORLD);
        }
    }
}

/*
    En cas de reception d'un message TERM, permet de recuperer l'identite du processus
    qui vient d'etre elu Leader et la liste de tous les id de l'anneau

    Ici le buffer contient :
    indice 0 : id_leader 
    indice 1 : index_list,
    indice 2 a BUFFER_SIZE-1 : donnee du tableau (list des id des autres processus)

    id : identite du processus courant
    VG : voisin gauche
    id_leader : identite du processus leader
    liste : On voudra copier la memoire du buffer de la liste des id des processus dans cette variable 
    pour le processus leader
*/
void recevoir_term(int buffer[], int id, int VG, int *id_leader, int liste[]) {
    int index_list = buffer[1];
    *id_leader = buffer[0];
    // buffer[1] represente l'index du tab a remplir a ce moment ci
    buffer[2+index_list] = id;
    buffer[1]++;
    if (buffer[0] != id) {
        MPI_Send(buffer, BUFFER_SIZE, MPI_INT, VG, TERM, MPI_COMM_WORLD);
    } else {
        buffer[2+buffer[1]] = id;
        memcpy(liste, buffer+2, sizeof(int) * buffer[1] + 1);
    }
}

/*
    Election d'un leader parmis tous nos processus pairs pour qu'il puisse par la suite nous 
    construire notre anneau unidirectionnel et la table des fingers pour chaque processus.

    rank : identifiant du processus 
*/
void build_dht(int rank) {

    MPI_Status status;

    // Reception des variables envoye par le simulateur, nous informent de notre voisin droit, gauche et 
    // de si ou non nous sommes initiateur

    int voisin_gauche;
    int voisin_droit;
    int is_initiateur;

    MPI_Recv(&voisin_gauche, 1, MPI_INT, MPI_ANY_SOURCE, INIT, MPI_COMM_WORLD, &status);
    MPI_Recv(&voisin_droit, 1, MPI_INT, MPI_ANY_SOURCE, INIT, MPI_COMM_WORLD, &status);
    MPI_Recv(&is_initiateur, 1, MPI_INT, MPI_ANY_SOURCE, INIT, MPI_COMM_WORLD, &status);     

    // Ici notre constante ID(i) est deja initialise et a comme nom rank

    // variables :

    // VD, VG et initiateur deja initialise au dessus 
    int etat = NSP;
    int k = 0;
    int nb_in = 0;

    // Le buffer de reception de tous types de messages
    int buffer[BUFFER_SIZE];

    // Juste un tableau de la taille du nombre de proc
    int liste_id[NR_NODE];
    int id_leader;

    // Les processus initiateurs declenchent la premiere etape de l'algorithme
    if (is_initiateur)
        initier_etape_suivante(k, &nb_in, rank, voisin_droit, voisin_gauche);

    // Traite chaque message en fonction de son type (TAG)
    while (1) {
        MPI_Recv(buffer, BUFFER_SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == IN) {
            recevoir_in(buffer, &etat, rank, voisin_droit, voisin_gauche, &nb_in, &k);
        } else if (status.MPI_TAG == OUT) {
            recevoir_out(buffer, &etat, rank, voisin_droit, voisin_gauche, is_initiateur);
        } else if (status.MPI_TAG == TERM) {
            // Il faut arreter l'algorithme quand un leader a ete elu, message TERM pour y parvenir
            recevoir_term(buffer, rank, voisin_gauche, &id_leader, liste_id);
            break;
        }
    }

    if (rank == id_leader) {
        
        // Construction de l'anneau unidirectionnel et le calcul centralise des fingers tables 
        int nb_pair = buffer[1];
        int list_final_id[NR_SIMU];
        list_final_id[0] = -1;
        memcpy(list_final_id+1, liste_id, sizeof(int) * nb_pair);

        for (int i = 0; i < NR_SIMU; i++) {
            printf("indice %d = %d\n", i, list_final_id[i]);
        }

        int id[NR_SIMU]; //node ids
        int finger_table[bits_of_keys+1][2]; //[MPI_id, finger_id]
        int first_hash = 0;
        int tmp[2];
        int srch_id = 0;

        srand(time(0));

        //id
        for (int i = 1; i < NR_SIMU; ++i) {
            id[i] = hash(rand());// % NR_KEY;
            //search if node id is unique
            if (i <= 1)
                continue;
            
            if (id[i-1] >= id[i])
                i = 0; //restart - another solution could be a qsort but it begins to be a bit complex with MPI ids.
        }

        //loop over each processes, and send them their the id and the finger table
        for (int i = 1; i < NR_SIMU; ++i) {
            MPI_Send(&id[i], 1, MPI_INT, list_final_id[i], INIT, MPI_COMM_WORLD);
            printf("--------------------\nnode_id: %d   |   MPI_id: %d\n", id[i], list_final_id[i]);

            //finger table
            for (int j = 1; j < bits_of_keys+1; ++j)  {
                find_id_finger(id, NR_SIMU, compute_finger(id[i], j-1), finger_table[j]);
                printf("\t[%d] :   MPI_id: %d   -   finger_id: %d\n", j-1, finger_table[j][0], finger_table[j][1]);
                MPI_Send(finger_table[j], 2, MPI_INT, list_final_id[i], INIT, MPI_COMM_WORLD);
            }

        }
    }
}

/*
    Construction d'un anneau bidirectionnel et choix aleatoire des processus initiateur (forcement
    au moins 1 processus initiateur)
*/
static void simulator(void) {
    MPI_Status status;

    // Construction de l'anneau bidirectionnel en passant les voisins gauches et droits

    int voisins_gauches[NR_SIMU] = {-1, NR_NODE, 1, 2, 3, 4, 5, 6};
    int voisins_droits[NR_SIMU] = {-1, 2, 3, 4, 5, 6, NR_NODE, 1};

    // Choix alatoires des processus initiateur 

    srand(getpid());
    int initiateurs[NR_SIMU];
    initiateurs[0] = 0;
    int has_init = 0;

    // On recommence tant qu'aucun processus n'a ete designe initiateur
    do {
        for (int i = 1; i < NR_SIMU; i++) {
        initiateurs[i] = rand() % 2;
        if (initiateurs[i])
            has_init = 1;
        }

        if (!has_init) {
            printf("Aucun processus initiateur : Rejeu!\n");
        }
    } while (!has_init);

    // On envoie les donnees ces donnes utiles au processus de notre anneau 
    for (int i = 1; i < NR_SIMU; i++) {
        MPI_Send(&voisins_gauches[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);    
        MPI_Send(&voisins_droits[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);
        MPI_Send(&initiateurs[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);    
    }
}

//main function - starting the simulator and algorithm
int main(int argc, char *argv[]) {
    int nr_proc, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nr_proc);

    // Si pas le bon nombre de processus arret
    if (nr_proc != NR_SIMU || NR_NODE > NR_KEY) {
        printf("config error\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Notre processus de rank 0 est notre processus simulateur, il ne fera donc pas partis de notre anneau
    if (rank == 0)
        simulator();
    else 
        build_dht(rank);
    
    MPI_Finalize();
    return EXIT_SUCCESS;
}