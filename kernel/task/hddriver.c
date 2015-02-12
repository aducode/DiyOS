/*******************************************************************/
/*	                  硬盘驱动                                 */
/*******************************************************************/
#include "type.h"
#include "proc.h"
#include "assert.h"
#include "hd.h"

static u8 hd_status;
static u8 hdbuf[SECTOR_SIZE * 2];

static void hd_identify(int drive);
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

void hd_identify(int drive)
{
/*	struct hd_cmd cmd;	
	cmd.device = MAKE_DEVICE_REG(0, drive, 0);
	cmd.command = ATA_IDENTIFY;
	hd_cmd_out(&cmd);
	interrupt_wait();
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	print_identify_info((u16*)hdbuf);		*/
}
