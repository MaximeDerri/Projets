// SPDX-License-Identifier: GPL-2.0
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

#include "interface_tm.h"

struct task_sample {
	uint64_t utime;
	uint64_t stime;
};

int main(int argc, char *argv[])
{
	struct task_sample tmp_ts;
	char tmp_b[BUFF_SIZE];
	unsigned char unused = 0;
	int pid;
	int fd = open("/dev/taskmonitor", O_RDWR);


	if (fd < 0) {
		perror("open");
		return -1;
	}

	//print
	if (ioctl(fd, TASKMONITORIOCG_BUFFER, tmp_b) < 0) {
		perror("ioctl");
		goto err;
	}
	printf("CHAR : %s\n", tmp_b);

	if (ioctl(fd, TASKMONITORIOCG_STRUCT, &tmp_ts) < 0) {
		perror("ioctl");
		goto err;
	}
	printf("STRUCT : usr %llu sys %llu\n", tmp_ts.utime, tmp_ts.stime);



	//stop - start * 2
	if (ioctl(fd, TASKMONITORIOCT_STOP, &unused) < 0) {
		perror("ioctl");
		goto err;
	}
	if (ioctl(fd, TASKMONITORIOCT_STOP, &unused) < 0) {
		perror("ioctl");
		goto err;
	}

	if (ioctl(fd, TASKMONITORIOCT_START, &unused) < 0) {
		perror("ioctl");
		goto err;
	}
	if (ioctl(fd, TASKMONITORIOCT_START, &unused) < 0) {
		perror("ioctl");
		goto err;
	}



	//test new pid
	pid = NEW_PID;
	if (ioctl(fd, TASKMONITORIOCT_SETPID, &pid) < 0) {
		perror("ioctl");
		goto err;
	}
	if (ioctl(fd, TASKMONITORIOCG_BUFFER, tmp_b) < 0) {
		perror("ioctl");
		goto err;
	}
	printf("CHAR : %s\n", tmp_b);

	pid = NEW_PID2;
	if (ioctl(fd, TASKMONITORIOCT_SETPID, &pid) < 0) {
		perror("ioctl");
		goto err;
	}
	if (ioctl(fd, TASKMONITORIOCG_BUFFER, tmp_b) < 0) {
		perror("ioctl");
		goto err;
	}
	printf("CHAR : %s\n", tmp_b);


	close(fd);
	return 0;

err:
	close(fd);
	return -1;

}
