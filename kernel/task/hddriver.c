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
//		printk("hd running...\n");
		//Main Loop start
		send_recv(RECEIVE, ANY, &msg); //接收广播消息，没有消息的时候阻塞
		int src = msg.source;
		switch(msg.type){
			case DEV_OPEN:
//				printk("hd receive DEV_OPEN message from %d\n", src);
				//hd_identify(0);
				hd_open(msg.DEVICE);
				break;
			case DEV_CLOSE:
//				printk("hd receive DEV_CLOSE message from %d\n", src);
				hd_close(msg.DEVICE);
				break;
			case DEV_READ:
			case DEV_WRITE:
//				printk("hd receive DEV_READ/DEV_WRITE message from %d\n", src);
				hd_rdwt(&msg);
				break;
			case DEV_IOCTL:
//				printk("hd receive DEV_IOCTL message from %d\n", src);
				hd_ioctl(&msg);
				break;
			default:
				dump_msg("HD driver::unknown msg",&msg);
				spin("FS::main_loop (invalid msg.type)");
				break;
		}
		send_recv(SEND, src, &msg);
	}
}

