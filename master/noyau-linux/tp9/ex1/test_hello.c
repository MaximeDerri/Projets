#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define LEN 1201

int main(int argc, char *argv[])
{
	int ret = 0;
	char buff[LEN];
	/*
	ret = syscall(548, "world"); //hello code
	printf("is syscall enable ? : %d\n", errno == (ENOSYS));
	printf("pid found in test_hello: %d\n", getpid());
	ret = syscall(548, 0xffffffffa00025c0, 1024); //q5
	//ret = syscall(548, "aeiouy", 1024); //q5
	*/
	ret = syscall(548, "des mots", buff, 8);
	if (ret < 0)
		printf("Error detected !\n");
	else
		printf("%s\n", buff);
	return ret;
}
