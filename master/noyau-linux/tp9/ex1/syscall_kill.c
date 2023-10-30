#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
	int ret = 0;
	
	ret = syscall(__NR_kill, getpid(), 9); //SIGKILL = 9
	printf("de retour\n");
	return ret;
}
