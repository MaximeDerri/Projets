// SPDX-License-Identifier: GPL-2.0
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "interface.h"


int main(int argc, char *argv[])
{
	char tmp[500];
	int fd = open("/dev/hello", O_RDWR);

	if (fd < 0) {
		perror("open");
		return -1;
	}

	if (argc != 2) {
		fprintf(stderr, "program takes a string as arg\n");
		return 0;
	}

	//print base
	if (ioctl(fd, HELLOIOCG, tmp) < 0) {
		perror("ioctl");
		goto err;
	}
	printf("rec = %s\n", tmp);

	//modifier la chaine
	if (ioctl(fd, HELLOIOCS, argv[1]) < 0) {
		perror("ioctl");
		goto err;
	}

	//reprint modif
	if (ioctl(fd, HELLOIOCG, tmp) < 0) {
		perror("ioctl");
		goto err;
	}
	printf("rec = %s\n", tmp);

	return 0;
	close(fd);


err:
	close(fd);
	return -1;
}