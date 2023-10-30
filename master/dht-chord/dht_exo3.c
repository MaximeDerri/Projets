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
#define UPDATE_REVERSE 4
#define UPDATE_PRED 5
#define UPDATE_FINGER 6
#define INSERT_NODE 7
#define INSERT_WAKEUP 8
#define SRCH_END 9

#define NR_INIT 6 //0:simulator; 1,2,...:node
#define NR_SIMU (NR_INIT+1) //+1: node to insert

#define NR_KEY 64 //2^n

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
	int reverse_table[NR_INIT][NR_KEY];
    int first_hash = 0;
	int tmp[2];
	int end = 0;

    srand(time(0));

	//default value for reverse_table
	for (int i = 0; i < NR_INIT; ++i) { //we create the ids of every nodes
		for (int j = 0; j < NR_KEY; ++j) {
			reverse_table[i][j] = -1;
		}
	}

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
	printf("-------------------- Finger table:\n");
    for (int i = 1; i < NR_INIT; ++i) {
        MPI_Send(&id[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);
		printf("--------------------\nnode_id: %d   |   MPI_id: %d\n", id[i], i);

        //finger table
        for (int j = 1; j < bits_of_keys+1; ++j)  {
            find_id_finger(id, NR_INIT, compute_finger(id[i], j-1), finger_table[j]);
			printf("\t[%d] :   MPI_id: %d   -   finger_id: %d\n", j-1, finger_table[j][0], finger_table[j][1]);
        	MPI_Send(finger_table[j], 2, MPI_INT, i, INIT, MPI_COMM_WORLD);
			reverse_table[finger_table[j][0]][id[i]] = i; //in the rever_table of the other node, at the id[i] place (id of current node), put my MPI_id i.
        }
    }

	//loop over each processes and send them their reverse table
	printf("-------------------- Reverse table:\n");
	for (int i = 1; i < NR_INIT; ++i) {
		printf("--------------------\nnode_id: %d   |   MPI_id: %d\n\t", id[i], i);
		for(int j = 0; j < NR_KEY; ++j) {
			if(reverse_table[i][j] > -1)
				printf("%d, ", j);
		}
		printf("\n");
		MPI_Send(reverse_table[i], NR_KEY, MPI_INT, i, INIT, MPI_COMM_WORLD);
	}
	printf("-------------------- Execution:\n");

	//wakeup a process to allow its insertion
	MPI_Send(&id[NR_INIT], 1, MPI_INT, NR_INIT, INSERT_WAKEUP, MPI_COMM_WORLD);
	MPI_Recv(&end, 1, MPI_INT, MPI_ANY_SOURCE, FINISH, MPI_COMM_WORLD, &status);
	for (int i = 1; i < NR_SIMU; ++i) {
		MPI_Send(&end, 1, MPI_INT, i, FINISH, MPI_COMM_WORLD);
	}
}

//finger_table[bits_of_keys][2];
//reverse_table[NR_KEY];
static int node_dht(int rank, int id, int finger_table[][2], int reverse_table[]) {
    MPI_Status status;
	int finger_updt[NR_KEY+2];
	int rec[2]; //[key, initiator]
	int send[2];
	int next = 0;
	int tmp = 0;
	int other_node_id = 0;
	int other_mpi_id = 0;

	//CHORD research algorithm
	while (1) {
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		switch (status.MPI_TAG) {
		case SRCH: //research key case
			MPI_Recv(rec, 2, MPI_INT, MPI_ANY_SOURCE, SRCH, MPI_COMM_WORLD, &status);
			if (id == rec[0]) {
				printf("\tnode_id == key - end\n");
				send[0] = id; //my node id
				send[1] = rank; //my mpi id
				MPI_Send(send, 2, MPI_INT, rec[1], SRCH_END, MPI_COMM_WORLD); //send SRCH_END to simulator
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
			MPI_Recv(rec, 2, MPI_INT, MPI_ANY_SOURCE, NOTIF_KEY, MPI_COMM_WORLD, &status);
			printf("\tSRCH has ended   -   node id: %d | key: %d\n", id, rec[0]);
			sleep(1);
			next = 0;
			send[0] = id; //my node id
			send[1] = rank; //my mpi id
			MPI_Send(&send, 2, MPI_INT, rec[1], SRCH_END, MPI_COMM_WORLD); //send SRCH_END to simulator

			break;
		case FINISH: //end
			MPI_Recv(rec, 1, MPI_INT, MPI_ANY_SOURCE, FINISH, MPI_COMM_WORLD, &status);
			return 0;

		case UPDATE_REVERSE: //reverse list update
			MPI_Recv(rec, 2, MPI_INT, MPI_ANY_SOURCE, UPDATE_REVERSE, MPI_COMM_WORLD, &status);
			printf("UPDATE_REVERSE, NODE ID:  %d\n", rank);
			reverse_table[rec[0]] = rec[1]; //in [node_id], put MPI_id

			break;
		case INSERT_NODE:
			MPI_Recv(rec, 2, MPI_INT, MPI_ANY_SOURCE, INSERT_NODE, MPI_COMM_WORLD, &status);
			//searching the predecessor
			other_node_id = -1;
			for (int i = (id == 0) ? NR_KEY-1 : id-1; i != id; --i) {
				if (reverse_table[i] != -1 && i != rec[0]) { //not the new node
					other_mpi_id = reverse_table[i];
					other_node_id = i;
					break;
				}
				if (i == 0) //loop
					i = NR_KEY;
			}
			//notify our predecessor for its new successor
			MPI_Send(rec, 2, MPI_INT, other_mpi_id,  UPDATE_PRED, MPI_COMM_WORLD);
			reverse_table[other_node_id] = -1; //-1 ==> will not be updated later

			//preparation for finger table updates
			memcpy(finger_updt, reverse_table, sizeof(int)*NR_KEY); //copy
			finger_updt[rec[0]] = -1; //the new node has its table up to date
			reverse_table[rec[0]] = rec[1]; //new predecessor
			finger_updt[NR_KEY] = id;
			finger_updt[NR_KEY+1] = rank;
			
			for (int i = 0; i < NR_KEY; ++i) {
				//we send the entire reverse list and leave this case because tmp can provoc a SRCH on this node
				//otherwise, it will result on a blockage
				if (finger_updt[i] > -1) { //every reverse except the new pred
					tmp = finger_updt[i];
					finger_updt[i] = -1;
					MPI_Send(finger_updt, NR_KEY+2, MPI_INT, tmp, UPDATE_FINGER, MPI_COMM_WORLD);
					break;
				}
			}
			//every reverse notifications will be treated now by UPDATE_REVERSE
			//a reverse update can be -1 because the index is no longer used, or > -1 to set a new one

			break;
		case UPDATE_PRED:
			MPI_Recv(rec, 2, MPI_INT, MPI_ANY_SOURCE, UPDATE_PRED, MPI_COMM_WORLD, &status);
			printf("IN UPDATE_PRED, NODE ID: %d\n", rank);
			sleep(1);
			other_node_id = rec[0];
			other_mpi_id = rec[1];
			next = finger_table[0][1]; //next 

			for (int i = 0; i < bits_of_keys; ++i) {
				//match ?
				if (finger_table[i][1] != next)
					continue;

				//change or not the finger table for the new interval
				tmp = compute_finger(id, i);
				if (id < tmp && tmp <= other_node_id) {
					finger_table[i][0] = other_mpi_id;
					finger_table[i][1] = other_node_id;
				}
				else if ((id < tmp && tmp < NR_KEY) || (tmp >= 0 && tmp <= other_node_id)) {
					finger_table[i][0] = other_mpi_id;
					finger_table[i][1] = other_node_id;
				}
			}
			sleep(1);

			break;
		case UPDATE_FINGER:
			MPI_Recv(finger_updt, NR_KEY+2, MPI_INT, MPI_ANY_SOURCE, UPDATE_FINGER, MPI_COMM_WORLD, &status);
			printf("IN UPDATE_FINGER, NODE ID: %d\n", rank);
			sleep(1);
			other_mpi_id = finger_updt[NR_KEY+1];
			other_node_id = finger_updt[NR_KEY];
			tmp = finger_table[0][0];

			for (int i = 0; i < bits_of_keys; ++i) {
				if (finger_table[i][1] == other_node_id) {
					send[0] = compute_finger(id, i);
					send[1] = rank;
					MPI_Send(send, 2, MPI_INT, tmp, SRCH, MPI_COMM_WORLD);
					MPI_Recv(rec, 2, MPI_INT, MPI_ANY_SOURCE, SRCH_END, MPI_COMM_WORLD, &status);
					//update reverse ?
					if (other_node_id != finger_table[i][1]) {
						send[0] = id;
						//notify the old finger
						send[1] = -1;
						MPI_Send(send, 2, MPI_INT, finger_table[i][0], UPDATE_REVERSE, MPI_COMM_WORLD);

						//notify the new finger
						send[1] = rank;
						MPI_Send(send, 2, MPI_INT, other_mpi_id, UPDATE_REVERSE, MPI_COMM_WORLD);

						//update finger_table
						finger_table[i][0] = other_mpi_id;
						finger_table[i][1] = other_node_id;

					}
				}
			}

			//continue ?
			tmp = -1;
			for (int i = 0; i < NR_KEY; ++i) {
				if (finger_updt[i] > -1) {
					tmp = finger_updt[i];
					finger_updt[i] = -1;
					MPI_Send(finger_updt, NR_KEY+2, MPI_INT, tmp, UPDATE_FINGER, MPI_COMM_WORLD);
					break;
				}
			}
			if (tmp < 0) {
				printf("UPDATE_FINGER COMPLETED ==> INSERTION DONE\n");
				MPI_Send(&tmp, 1, MPI_INT, 0, FINISH, MPI_COMM_WORLD);
			}

			break;
		default:
			printf("Unknown tag received\n");
		}
	}

    return 0;
}

static int node_start(int rank) {
    MPI_Status status;
    int finger_table[bits_of_keys][2];
	int reverse_table[NR_KEY];
    int id = 0;

    //init
    MPI_Recv(&id, 1, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	for (int i = 0; i < bits_of_keys; ++i) {
    	MPI_Recv(finger_table[i], 2, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
	}
	MPI_Recv(reverse_table, NR_KEY, MPI_INT, 0, INIT, MPI_COMM_WORLD, &status);
    return node_dht(rank, id, finger_table, reverse_table);
}

static int node_insert(int rank) {
	MPI_Status status;
	int finger_table[bits_of_keys][2]; //[MPI_id, finger_id]
	int reverse_table[NR_KEY]; //[i] = x ==> i: node_id, x: mpi_id
	int send[2];
	int rec[2];
	int id = 0;
	int srch_target = 0;
	int succ_mpi_id = 0;

	//init
	for (int i = 0; i < NR_KEY; ++i) {
		reverse_table[i] = -1;
	}

	//receive our node id
	MPI_Recv(&id, 1, MPI_INT, 0, INSERT_WAKEUP, MPI_COMM_WORLD, &status);

	//searching our successor for the futur insertion
	send[0] = id;
	send[1] = rank;
	srch_target = 1 + (rand() % (NR_INIT-1));
	printf("SEARCHING A PLACE FOR INSRTION OF NODE ID %d\n", id);
	MPI_Send(send, 2, MPI_INT, srch_target, SRCH, MPI_COMM_WORLD);
	MPI_Recv(rec, 2, MPI_INT, MPI_ANY_SOURCE, SRCH_END, MPI_COMM_WORLD, &status);
	succ_mpi_id = rec[1];

	//building our finger table
	printf("BUILDING FINGER TABLE BEFOR INSERTION OF ID NODE %d | SUCCESSOR: %d\n", id, rec[0]);
	for (int i = 0; i < bits_of_keys; ++i) {
		send[0] = compute_finger(id, i);
		MPI_Send(send, 2, MPI_INT, srch_target, SRCH, MPI_COMM_WORLD);
		MPI_Recv(rec, 2, MPI_INT, MPI_ANY_SOURCE, SRCH_END, MPI_COMM_WORLD, &status);
		finger_table[i][0] = rec[1];
		finger_table[i][1] = rec[0];

		//notify our finger for their reverse table
		send[0] = id;
		MPI_Send(send, 2, MPI_INT, rec[1], UPDATE_REVERSE, MPI_COMM_WORLD);
	}

	//print
	printf("--------------------\nnode_id: %d   |   MPI_id: %d\n", id, rank);
	for (int i = 0; i < bits_of_keys; ++i) {
		printf("\t[%d] :   MPI_id: %d   -   finger_id: %d\n", i, finger_table[i][0], finger_table[i][1]);
	}
	sleep(1);

	//notify the successor
	MPI_Send(send, 2, MPI_INT, succ_mpi_id, INSERT_NODE, MPI_COMM_WORLD);
	return node_dht(rank, id, finger_table, reverse_table);
}


//main function - starting the simulator and algorithm
int main(int argc, char *argv[]) {
    int nr_proc, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nr_proc);

    if (nr_proc != NR_SIMU || NR_SIMU-1 > NR_KEY) {
        printf("config error\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
        simulator();
    else if (rank < NR_INIT)
        node_start(rank);
	else
		node_insert(rank);

    MPI_Finalize();
    return EXIT_SUCCESS;
}
