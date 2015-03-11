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
	printf("init() is running ...\n");
	int pid = fork();
	if(pid!=0){
		printf("parent is running, pid:%d, child pid:%d\n", getpid(), pid);
	//	int s;
	//	int child = wait(&s);
	//	printf("child (%d) exited with status:%d.\n", child, s);
	} else {
		printf("child is running, pid:%d\n", getpid());
/*		pid = fork();
		if(pid!=0){
			//int ss;
			printf("xxxxx\n");
			//int cc = wait(&ss);
			//printf("%d, %d\n", cc, ss);
		} else {
			printf("yyyyyy\n");
		//	exit(333);
		}
*/
		exit(123);
	}
	//init进程要不断的wait，因为如果一个exit的进程有子进程，那么会被转移成init的子进程，这些子进程exit时，init会wait
	while(1){
		int s;
		int child= wait(&s);
		if(child>=0){
			printf("child (%d) exited with status:%d.\n", child, s);
		}
		//如果wait返回-1表示init进程没有子进程
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
