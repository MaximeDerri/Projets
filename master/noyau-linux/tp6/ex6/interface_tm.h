#ifndef INTERFACE_TM
#define INTERFACE_TM


#define BUFF_SIZE 128
#define MAGIC 'N'
#define NEW_PID 5
#define NEW_PID2 9999
#define TASKMONITORIOCG_BUFFER _IOR(MAGIC, 0, char *) //G: Get, buffer
#define TASKMONITORIOCG_STRUCT _IOR(MAGIC, 1, struct task_sample *) //G: Get, struct task_sample
#define TASKMONITORIOCT_STOP _IOW(MAGIC, 2, unsigned char) //T: tell, no arg (0)
#define TASKMONITORIOCT_START _IOW(MAGIC, 3, unsigned char) //T: tell, no arg(0)
#define TASKMONITORIOCT_SETPID _IOW(MAGIC, 4, int) //T: tell, new pid

#endif