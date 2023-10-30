#ifndef DSM_H
#define DSM_H

#include "dsm.h"
#include "master.h"
#include "slave.h"
#include "lock_demand.h"

void *InitMaster(int size);
void *InitSlave(char *host_master);
void LoopMaster(void);
void lock_read(void *adr, int s);
void unlock_read(void *adr, int s);
void lock_write(void *adr, int s);
void unlock_write(void *adr, int s);

#endif