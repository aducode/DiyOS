#include "type.h"
#include "stdio.h"
#include "stat.h"
#include "mount.h"
#include "assert.h"
#include "fork.h"
#include "getpid.h"
#include "concurrent.h"
//#include "tar.h"

//#define _WITH_TEST_	//MOVE TO type.h
#ifndef _WITH_TEST_
void init(){
	//为了简化，这里stdin  stdout stderr只是简单的作为FD
	//真正的linux中stdin stdout stderr 是作为FILE *类型存在的
	int stdin = open("/dev/tty0", O_RDWT);
	assert(stdin==0);
	int stdout = open("/dev/tty0", O_RDWT);
	assert(stdout==1);
	int stderr = open("/dev/tty0", O_RDWT);
	assert(stderr==2);
	//创建/root 作为进程目录
	mkdir("/root");
	//设置进程目录
	chdir("/root");
	//由于init进程设置了进程运行时目录，所以所有用户态的进程都会继承这个目录值
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
static void test_fs(int id);
static void test_fs2(int id);
static void test_fork(int id);
static void test_mount(int id);
static void test_sleep(int id);
static int get_inode_icnt(const char *pathname);
void init()
{
	int stdin = open("/dev/tty0", O_RDWT);
	assert(stdin==0);
	int stdout = open("/dev/tty0", O_RDWT);
	assert(stdout == 1);
	int stderr = open("/dev/tty0", O_RDWT);
	assert(stderr == 2);
	//创建/root 作为进程目录
	mkdir("/root");
	//设置进程目录
	chdir("/root");
	//test /dev/floppy
	//open("/dev/floppy", O_RDWT);
	//assert(0);
	printf("RUN TESTS...\n");
	test_fs(1);
	test_fs2(2);
	test_fork(3);
	test_mount(4);
	//test_sleep(4);
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
void test_fs(int id)
{
	int ret, fd, bytes, pos;
	char buf[50];
	char file_content[] = "123issac!@#$%^&*()_+";
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
	ret = stat("/hello", &statbuf);
	assert(ret == 0);
	printf("[%d]TEST FILE SUCCESS!!\n", id);
}

/**
 * @function test_fs2
 * @brief 测试用例
 *
 * @return void
 */
void test_fs2(int id)
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
	printf("[%d]TEST DIRECTORY SUCCESS!!\n", id);
}

void test_fork(int id)
{
	//int i;
	int pid = fork();
	assert(pid>=0);
        if(pid!=0){
		//for(i=0;i<30;i++){
		//	printf("P%d,",i);
		//}
                int s;
                int cpid = wait(&s);
		assert(s == 2333);
        } else {
		//for(i=0;i<20;i++) printf("C%d,",i);	
		/*
		int i;
		for(i=0;i<10;i++){
			sleep((i+1)*1000);
		}
		*/
                exit(2333);
        }
	printf("[%d]TEST FORK/EXIT SUCCESS!!\n", id);

}

void test_mount(int id)
{
	int ret;
	ret = mkdir("/root/tmp");
	assert(ret==0);
	ret = mount("/dev/floppy", "/root/tmp");
	printf("ret=%d\n", ret);
	ret = unmount("/root/tmp");
	printf("ret=%d\n", ret);
	ret = rmdir("/root/tmp");
	assert(ret==0);
	printf("[%d]TEST MOUNT SUCCESS!!\n", id);
}

void test_sleep(int id)
{
	int i;
	for(i=0;i<10;i++){
		printf(".");
		sleep(i*1000);
	}
	printf("\n");
	printf("[%d]TEST SLEEP SUCCESS!!\n", id);
}

int get_inode_icnt(const char *pathname)
{
	struct stat statbuf;
	int ret = stat(pathname, &statbuf);
	assert(ret == 0);
	return statbuf.i_cnt;
}
#endif
