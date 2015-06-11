#include "type.h"
#include "stdio.h"
#include "assert.h"
#include "fork.h"
#include "getpid.h"
#include "tar.h"

#define _FOR_TEST_
#ifndef _FOR_TEST_
void init(){
	int stdin = open("/dev_tty0", O_RDWT);
	assert(stdin==0);
	int stdout = open("/dev_tty0", O_RDWT);
	assert(stdout==1);
	int stderr = open("/dev_tty0", O_RDWT);
	assert(stderr==2);
	char buf[81];
	int bytes;
	while(1){
		printf("$");
		bytes=read(stdin, buf, 79);
		buf[bytes] = 0;
		if(buf[0]){
			printf(">%s\n", buf);
		}	
	}
}
#else
void test_fs();
void init()
{
//	_disp_str("Yooooooooooo",0,0, COLOR_RED);
	//下面打开stdin stdout了会死锁
	//TODO fix it
	int stdin = open("/dev_tty0", O_RDWT);
	assert(stdin==0);
//	_disp_str("open stdin success", 2, 0, COLOR_GREEN);
	int stdout = open("/dev_tty0", O_RDWT);
	assert(stdout == 1);
//	_disp_str("open stdout success", 4, 0, COLOR_GREEN);
	test_fs();
	untar("/cmd.tar");
	int pid = fork();
	if(pid!=0){
		printf("parent is running, pid:%d child pid:%d\n", getpid(), pid);
		//int pid2 = fork();
		//if(pid2!=0){
				
		//} else {
		//	printf("child is running, pid:%d\n", getpid());
		//}
		int s;
		int cpid = wait(&s);
		printf("child %d exit:%d\n", cpid, s);
	} else {
		printf("child is running, pid:%d\n", getpid());
		//int pid3 = fork();
		//if(pid3!=0){
		//	printf("parent is running, pid:%d, child pid:%d\n", getpid(), pid);
		//} else {
		//	printf("child is running, pid:%d\n", getpid());
		//}
		exit(2333);
	}
	char buf[128];
	while(1){
		int status;
		int child = wait(&status);
		if(child>=0){
			printf("child %d exit:%d\n",child, status);	
		}
		printf("$");
		int l = read(stdin, buf, 70);
		buf[l] = 0;
		if(buf[0]){
			printf(">%s\n", buf);	
		}
	}
/*
//	test_fs();
//	untar("/cmd.tar");	
	int pid = fork();
	if(pid!=0){
//		printf("parent is running, pid:%d, child pid:%d\n", getpid(), pid);
//		printf("parent is running\n");
		int ppid = fork();
		if(ppid!=0){
			printf("parent is running...\n");
		} else {
			printf("child is running, pid:%d\n", getpid());
                        exit(111);
		}
	//	int s;
	//	int child = wait(&s);
	//	printf("child (%d) exited with status:%d.\n", child, s);
	} else {
		printf("child is running, pid:%d\n", getpid());
		pid = fork();
		if(pid!=0){
			//int ss;
			printf("xxxxx\n");
			//int cc = wait(&ss);
			//printf("%d, %d\n", cc, ss);
		} else {
			printf("yyyyyy\n");
		//	exit(333);
		}
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
*/
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

/**
 * @function test_fs
 * @brief 测试文件系统
 */
void test_fs()
{
	int fd;
	fd = open("/test.txt", O_CREATE|O_RDWT);
	assert(fd!=-1);
	write(fd,"0987654321",10);
	int pos = tell(fd);
	printf("----pos:%d\n", pos);
	seek(fd,0,SEEK_START);
	pos = tell(fd);
	printf("----pos:%d\n", pos);
	char buf[11];
	int bytes = read(fd, buf, 10);
	buf[bytes]=0;
	printf("%s\n", buf);
	close(fd);
	printf("read data from cmd.tar\n");
	fd = open("/cmd.tar", O_RDWT);
	while(1){
		bytes = read(fd,buf, 10);
		buf[bytes]=0;
		if(buf[0]){
			printf("%s\n",buf);
		} else {
			printf("read data from cmd.tar END\n");
			break;
		}
	}
	//fd = open("/test.txt", O_RDWT);
	//char buf[11];
	//int bytes = read(fd, buf, 10);
	//buf[bytes]=0;
	//printf(buf);
	//close(fd);
}
#endif
