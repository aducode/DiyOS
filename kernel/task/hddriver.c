/*******************************************************************/
/*	                  硬盘驱动                                 */
/*******************************************************************/
#include "type.h"
#include "syscall.h"
#include "proc.h"
#include "assert.h"
#include "hd.h"
/**
 * Main Loop of HD driver
 */
void task_hd()
{
	struct message msg; //用于进程间通信，存储消息体
	//初始化硬盘设备
	init_hd();
	while(1){
		//Main Loop start
		send_recv(RECEIVE, ANY, &msg); //接收广播消息，没有消息的时候阻塞
		int src = msg.source;
		switch(msg.type){
			case DEV_OPEN:
				//hd_identify(0);
				hd_open(msg.DEVICE);
				break;
			default:
				dump_msg("HD driver::unknown msg",&msg);
				spin("FS::main_loop (invalid msg.type)");
				break;
		}
		send_recv(SEND, src, &msg);
	}
}

