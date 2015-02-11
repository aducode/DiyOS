#include "type.h"
#include "stdio.h"
#include "assert.h"
#include "string.h"
#include "proc.h"
#include "stdlib.h"
/**
 * 使用sendrec系统调用
 */
long long get_ticks()
{
        struct message msg;
        reset_msg(&msg);
        msg.type = GET_TICKS;
        send_recv(BOTH, 1, &msg);
        return msg.RETVAL;
}

/********************* testA  testB 是ring1级别的 ***************************/
void testA()
{
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
        while(1){
//		printf("%d\n", get_ticks());
//		printf("hello in testA\n");	
//                _disp_str("IN RING1 PROC A...    ticks now is:",0,0, COLOR_GREEN);
//                //_clean(0,0,25,80);
//                itoa(ticks,msg,10);
//                _disp_str(msg,1,0, COLOR_GREEN);
//              _hlt();                 //将运行在ring1 ，ring1执行hlt会报异常
        }
}
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
