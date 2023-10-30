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

#define NR_NODE 7
#define NR_SIMU (NR_NODE+1)

#define NR_KEY 64 //2^n

static int bits_of_keys = (int)log2l(NR_KEY);

//ex3
//#define INS_NODE X
//#define UPD_FT Y //update finger table
//#define NR_INIT Z


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

//MPI id are send for communication...
/**
 * send:
 * node id
 * finger table
 * ==>to each processes except 0
*/
static void simulator(void) {
	MPI_Status status;
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
        MPI_Send(&id[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);
		printf("--------------------\nnode_id: %d   |   MPI_id: %d\n", id[i], i);

        //finger table
        for (int j = 1; j < bits_of_keys+1; ++j)  {
            find_id_finger(id, NR_SIMU, compute_finger(id[i], j-1), finger_table[j]);
			printf("\t[%d] :   MPI_id: %d   -   finger_id: %d\n", j-1, finger_table[j][0], finger_table[j][1]);
        	MPI_Send(finger_table[j], 2, MPI_INT, i, INIT, MPI_COMM_WORLD);
        }

    }
	printf("--------------------\nExecution:\n");

	//launch a research
	srch_id = 1 + (rand() % (NR_SIMU-1));
	sleep(1);
	tmp[0] = hash(rand());
	tmp[1] = 0;
	printf("----------\nnode target: %d\nkey: %d\n----------\n\n", id[srch_id], tmp[0]);
	sleep(1);
	MPI_Send(tmp, 2, MPI_INT, srch_id, SRCH, MPI_COMM_WORLD);

	//wait for the end
	MPI_Recv(&id, 1, MPI_INT, MPI_ANY_TAG, FINISH, MPI_COMM_WORLD, &status);
	for (int i = 1; i < NR_SIMU; ++i) {
		MPI_Send(tmp, 2, MPI_INT, i, FINISH, MPI_COMM_WORLD);
	}
}

static int node_dht(int rank) {
    MPI_Status status;
    int finger_table[bits_of_keys][2];
	int rec[2]; //[key, initiator]
    int id = 0;
	int run = 1;
	int next = 0;

    //init
    MPI_Recv(&id, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	for (int i = 0; i < bits_of_keys; ++i) {
    	MPI_Recv(finger_table[i], 2, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	}

    //CHORD research algorithm
	while (run) {
		MPI_Recv(rec, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch (status.MPI_TAG) {
		case SRCH: //research key case
			if (id == rec[0]) {
				printf("\tnode_id == key - end\n");
				MPI_Send(&next, 1, MPI_INT, rec[1], FINISH, MPI_COMM_WORLD); //send FINISH to simulator
				break;
			}
			next = -1;
			//iterate over finger table
			for (int i = bits_of_keys - 1; i > 0; --i)  {
				//case when P_i < key and we have a 'normal' interval - 0 is not between P_i and finger
				if (id < finger_table[i][1]) {
					if (!(rec[0] > id && rec[0] <= finger_table[i][1])) {
						next = i;
						break; //found
					}
				}
				//P_i / 0 / P_j case, we use 2 different intervals to decide
				else {
					if (!((rec[0] > id && rec[0] < NR_KEY) || (rec[0] >= 0 && rec[0] <= finger_table[i][1]))) {
						next = i;
						break; //found
					}
				}
			}

			//forward or notify the next node which has the key in is range
			if (next < 0) { //notify
				printf("\tSRCH   -   node id: %d | key: %d   -   NOTIFY END TO %d\n", id, rec[0], finger_table[0][1]);
				sleep(1);
				MPI_Send(rec, 2, MPI_INT, finger_table[0][0], NOTIF_KEY, MPI_COMM_WORLD);
			}
			else { //forward
				printf("\tSRCH   -   node id: %d | key: %d   -   FORWARD TO %d\n", id, rec[0], finger_table[next][1]);
				sleep(1);
				MPI_Send(rec, 2, MPI_INT, finger_table[next][0], SRCH, MPI_COMM_WORLD);
			}

			break;
		case NOTIF_KEY: //notif to inform the node that it has the key
			printf("\tSRCH has ended   -   node id: %d | key: %d\n", id, rec[0]);
			sleep(1);
			next = 0;
			MPI_Send(&next, 1, MPI_INT, rec[1], FINISH, MPI_COMM_WORLD); //send FINISH to simulator
			break;
		case FINISH: //end
			run = 0;
			break;
		default:
			printf("Unknown tag received\n");
		}
	}

    return 0;
}


//main function - starting the simulator and algorithm
int main(int argc, char *argv[]) {
    int nr_proc, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nr_proc);

    if (nr_proc != NR_SIMU || NR_NODE > NR_KEY) {
        printf("config error\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
        simulator();
    else
        node_dht(rank);

    MPI_Finalize();
    return EXIT_SUCCESS;
}
