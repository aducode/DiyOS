/*******************************************************************/
/*	                  硬盘驱动                                 */
/*******************************************************************/
#include "type.h"
#include "proc.h"
#include "assert.h"
static void init_hd();
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
		int src = msg.sources;
		switch(msg.type){
			case DEV_OPEN:
				hd_identify(0);
				break;
			default:
				dump_msg("HD driver::unknown msg",&msg);
				spin("FS::main_loop (invalid msg.type)");
				break;
		}
		send_recv(SEND, src, &msg);
	}

}

/**
 * 初始化硬盘
 */
void init_hd()
{
	//Get the number of drives from the BIOS data area
}
