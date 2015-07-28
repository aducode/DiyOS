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
static void test_fs();
static void test_fs2();
static void test_fork();
void init()
{
	int stdin = open("/dev/tty0", O_RDWT);
	assert(stdin==0);
	int stdout = open("/dev/tty0", O_RDWT);
	assert(stdout == 1);
	int stderr = open("/dev/tty0", O_RDWT);
	assert(stderr == 2);
	//test /dev/floppy
	//open("/dev/floppy", O_RDWT);
	//assert(0);
	test_fs();
	//TODO 测试目录操作过程中有一些问题
	//test_fs2();
	test_fork();
	//untar("/cmd.tar");
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
 *
 * @return  void
 */
void test_fs()
{
	int ret, fd, bytes, pos;
	char buf[50];
	char file_content[] = "issac!@#$%^&*()_+";
	int file_content_len = strlen(file_content);
	ret = mkdir("/hello");
	assert(ret==0);
	ret = mkdir("/hello/fuckyou");
	assert(ret == 0);
	ret = mkdir("/hello/fuckyou/test");
	assert(ret == 0);
	fd = open("/hello/fuckyou/test/test.txt", O_CREATE|O_RDWT);
	assert(fd==3);
	bytes = write(fd,file_content, file_content_len);
	assert(bytes == file_content_len);
	pos = tell(fd);
	assert(pos == file_content_len);
	ret = seek(fd,0,SEEK_START);
	assert(ret == 0);
	pos = tell(fd);
	assert(pos == 0);
	ret = close(fd);
	assert(ret == 0);
	fd = open("/hello/fuckyou/test/test.txt", O_RDWT);
	assert(fd == 3);
	bytes = read(fd, buf, file_content_len);
	assert(bytes == file_content_len);
	buf[bytes]=0;
	assert(strcmp(buf,file_content) == 0); 
	ret = close(fd);
	assert(ret == 0);
	struct stat statbuf;
	ret = stat("/hello/fuckyou/test/test.txt", &statbuf);
	assert(ret == 0);
	assert(statbuf.st_size == file_content_len);
}

/**
 * @function test_fs2
 * @brief 测试用例
 *
 * @return void
 */
void test_fs2()
{
	int ret, fd;
	//1. test for mkdir
	ret = mkdir("/dir");
	//here is should success
	assert(ret==0);
	//2. test for create file
	fd = open("/dir/yoo", O_CREATE);
	//fd should be gt 0
	assert(fd>0);
	//3. test for close file	
	ret = close(fd);
	//ret should success
	assert(ret == 0);
	//4. test for delte not empty directory
	ret = rmdir("/dir");
	//should fail, because it not empty
	assert(ret == -1);
	//5. test for delete file
	ret = unlink("/dir/yoo");
	//should success
	assert(ret == 0);
	//6. test open not exists file 	
	fd = open("/dir/yoo", O_RDWT);
	//should fail
	assert(fd == -1);
	//7. test remove empty directory
	ret = rmdir("/dir");
	//should success
	assert(ret == 0);
	//8. test create file in not existed directory
	fd = open("/dir/yoo", O_CREATE);
	//should fail
	assert(fd == -1);
}

void test_fork()
{
	int pid = fork();
	assert(pid>=0);
        if(pid!=0){
                int s;
                int cpid = wait(&s);
		assert(s == 2333);
        } else {
                exit(2333);
        }

}
#endif
