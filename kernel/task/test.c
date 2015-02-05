#include "klib.h"
#include "global.h"
#include "string.h"
#include "stdio.h"
/********************* testA  testB 是ring1级别的 ***************************/
void testA()
{
	_disp_str("start....",20,0,COLOR_RED);
	//printf("abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789\t");
	printf("testA\ntestA\t1234567890\tabcdefg\t%o\n", 12);
       // static char msg[20];
        while(1){
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
	printf("aaaab testB %d\n", 11);
        //static char msg[20];
        while(1){
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
	printf("adfadfadfadfasd testC %x\n",17);
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
	printf("adfadfadfa dfadfadfasdf testD\n");
	while(1){
	//	printf("testD, \n%s\n","hehehehe");
	}
}             
