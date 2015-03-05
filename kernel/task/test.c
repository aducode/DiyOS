#include "type.h"
#include "stdio.h"
#include "assert.h"
#include "string.h"
#include "proc.h"
#include "systicks.h"
#include "klib.h"
/********************* testA  testB 是ring1级别的 ***************************/
void testA()
{	
	_disp_str("testA...", 24, 0, COLOR_WHITE);
	//0x7e96
	int fd = open("/hehe",O_CREATE|O_RDWT);
	int cnt = write(fd,"*****!!!FUCK#Y0u\n",10);
	close(fd);
	fd = open("/hehe",O_RDWT);
	char data[200];
	read(fd, data, cnt);
	printf(data);
	close(fd);
//	printf("0x%x\n",111);
//	assert(0);
//	printf("abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789\t");
//	printf("ticks:%d", get_ticks());	
//	char msg[200];
//	char msg2[200];
//	memset(msg, 'a',10);
//	msg[10]='\0';
//	printf(msg);
//	memcpy(msg2, msg, 11);
//	printf("\n");
//	printf(msg2);
//	printf("%d\n",get_ticks());
       // static char msg[20];

	printf("testA running .... %d\n", (int)get_ticks());
/*
	printf("-1=%d\n",-1);
	int fd = open("/dev_tty0",O_RDWT);
	char *str = "hello world\nYooooooooooooooooooo~~~~~~~~~~~~~~\n";
	int cnt = write(fd, str, strlen(str));
	char data[256];
	close(fd);
	fd = open("/dev_tty0", O_RDWT);
	read(fd,data, cnt);
	printf(data);
	close(fd);
*/
//	int fd = open("/blah",O_CREATE|O_RDWT);
//	printf("/blah fd:%d\n", fd);
//	char *data="hello world\n";
//	int cnt = write(fd, data, strlen(data));
//	char *data1 = "fuck you~~~~~~~~~~~~~~\n";
//	cnt+=write(fd, data1, strlen(data1));
//	printf("write %d bytes\n", cnt);
//	close(fd);
//	fd = open("/blah",O_RDWT);
//	char data2[200];
//	read(fd, data2, cnt);
//	if(cnt>199){
//		cnt = 199;
//	}
//	data2[cnt]='\0';
//	printf("read data from /blah:\n%s\n", data2);
//	close(fd);
//	int ret = unlink("/blah");
//	printf("unlink /blah, result:%d\n",ret);
//	fd = open("/blah",O_RDWT);
//	printf("open /blah fd:%d\n",fd);
//	read(fd, data2, cnt);
//	printf("%s\n",data2);
//	close(fd);
        while(1){
//		printf("%d,",(int)get_ticks());
//		printf("%d\n", get_ticks());
//		printf("hello in testA\n");	
//                _disp_str("IN RING1 PROC A...    ticks now is:",0,0, COLOR_GREEN);
//                //_clean(0,0,25,80);
//                itoa(ticks,msg,10);
//                _disp_str(msg,1,0, COLOR_GREEN);
//              _hlt();                 //将运行在ring1 ，ring1执行hlt会报异常
        }
}
/*
void testB()
{
	printf("testB running...%d\n",(int)get_ticks());
//	int t=(int)get_ticks();
//	printf("after get_ticks()\t%d\n",(int)get_ticks());//如果有多个用户线程，这里就会阻塞
        //static char msg[20];
        while(1){
//	printf("%d,",get_ticks());//如果在循环里调用get_ticks()会出现Page Fault异常
//		printf("testB %d\n",10);
  //               _disp_str("IN RING1 PROC B...    ticks now is:",2,0,COLOR_WHITE);
   //             itoa(ticks, msg,10);
    //            _disp_str(msg,3,0, COLOR_WHITE);
//              _hlt();
//                get_ticks();
        }
}
void testC()
{
	printf("testC running ...%d\n",(int)get_ticks());
        //static char msg[20];
        while(1)
        {
	//	printf("testC %x\n",17);
     //           _disp_str("IN RING1 PROC C...    ticks now is:",4,0,COLOR_YELLOW);
      //          itoa(ticks, msg, 10);
       //         _disp_str(msg, 5,0,COLOR_YELLOW);
        }
}

void testD(){
	printf("testD running ...%d\n", (int)get_ticks());
	while(1){
	//	printf("testD, \n%s\n","hehehehe");
	}
}
*/
