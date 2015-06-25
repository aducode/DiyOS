#include "type.h"
#include "stdio.h"
#include "assert.h"
#include "fork.h"
#include "getpid.h"
#include "tar.h"

#define _FOR_TEST_
#ifndef _FOR_TEST_
void init(){
	int stdin = open("/dev/tty0", O_RDWT);
	assert(stdin==0);
	int stdout = open("/dev/tty0", O_RDWT);
	assert(stdout==1);
	int stderr = open("/dev/tty0", O_RDWT);
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
	int stdin = open("/dev/tty0", O_RDWT);
	assert(stdin==0);
	int stdout = open("/dev/tty0", O_RDWT);
	assert(stdout == 1);
	test_fs();
	//untar("/cmd.tar");
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
}


/**
 * @function test_fs
 * @brief 测试文件系统
 */
void test_fs()
{
	char file_content[] = "!@#$%^&*()_+";
	int file_content_len = strlen(file_content);
	mkdir("/hello");
	//这里连续两个mkdir后，下面write就会出错
	//HD_TASK有问题
	mkdir("/hello/fuckyou");
	int fd;
	fd = open("/hello/fuckyou/test.txt", O_CREATE|O_RDWT);
	assert(fd!=-1);
	write(fd,file_content, file_content_len);
	//int pos = tell(fd);
	//printf("----pos:%d\n", pos);
	//seek(fd,0,SEEK_START);
	//pos = tell(fd);
	//printf("----pos:%d\n", pos);
	char buf[50];
	int bytes;
	//int bytes = read(fd, buf, file_content_len);
	//buf[bytes]=0;
	//printf("%s\n", buf);
	close(fd);
	fd = open("/hello/fuckyou/test.txt", O_RDWT);
	bytes = read(fd, buf, file_content_len);
	buf[bytes]=0;
	printf("%s\n",buf);
	close(fd);
}
#endif
