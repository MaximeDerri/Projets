#include "dsm.h"

void *InitMaster(int size) {
    return init_master(size);
}

void *InitSlave(char *host_name) {
    return init_slave(host_name, PORTSERV);
}

void LoopMaster(void) {
    loop_master();
}

void lock_read(void *adr, int size) {
    lock(adr, size, LOCKR);
}

void lock_write(void *adr, int size) {
    lock(adr, size, LOCKW);
}

void unlock_read(void *adr, int size) {
    lock(adr, size, UNLOR);
}

void unlock_write(void *adr, int size) {
    lock(adr, size, UNLOW);
}