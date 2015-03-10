#include "type.h"
#include "stdio.h"
#include "assert.h"
#include "fork.h"
#include "getpid.h"
void init()
{
	int stdin = open("/dev_tty0", O_RDWT);
	assert(stdin==0);
	int stdout = open("/dev_tty0", O_RDWT);
	assert(stdout == 1);
	printf("Init is running ...\n");
	int pid = fork();
	if(pid!=0){
		printf("parent is running, child pid:%d\n", pid);
		spin("parent");
	} else {
		printf("child is running, pid:%d\n", getpid());
		char rdbuf[128];
		while(1){
			printf("$");
			int r = read(stdin, rdbuf, 70);
			rdbuf[r]=0;
			if(rdbuf[0]){
				printf(">%s\n", rdbuf);
			}
		}
	}
}
/*
void init(){
	int stdin = open("/dev_tty0", O_RDWT);
	assert(stdin == 0);
	int stdout = open("/dev_tty0", O_RDWT);
	assert(stdout == 1);
	int stderr = open("/dev_tty0", O_RDWT);
	assert(stderr == 2);
	char rdbuf[128];
        while(1){
		printf("$");
		int r =	 read(stdin, rdbuf, 70);
		rdbuf[r] = 0;
		if(rdbuf[0]){
			printf(">%s\n", rdbuf);
		}	
 	}
	close(stderr);
	close(stdout);
	close(stdin);   
}
*/
