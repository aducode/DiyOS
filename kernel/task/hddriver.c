/*******************************************************************/
/*	                  硬盘驱动                                 */
/*******************************************************************/
#include "type.h"
#include "klib.h"
#include "proc.h"
#include "assert.h"
#include "systicks.h"
#include "hd.h"
#include "clock.h"
static u8 hd_status;
static u8 hdbuf[SECTOR_SIZE * 2];

static void interrupt_wait();
static int waitfor(int mask, int val, int timeout);
static void hd_cmd_out(struct hd_cmd *cmd);
static void hd_identify(int drive);


static void print_identify_info(u16* hdinfo);
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
	struct hd_cmd cmd;	
	cmd.device = MAKE_DEVICE_REG(0, drive, 0);
	cmd.command = ATA_IDENTIFY;
	hd_cmd_out(&cmd);
	interrupt_wait();
	_port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	print_identify_info((u16*)hdbuf);
}

void print_identify_info(u16 *hdinfo)
{
	int i, k;
	char s[64];
	struct iden_info_ascii{
		int idx;
		int len;
		char *desc;
	} iinfo[]={{10,20,"HD SN"},{27,40,"HD Model"}};
	for(k=0;k<sizeof(iinfo)/sizeof(iinfo[0]);k++){
		char *p = (char*)&hdinfo[iinfo[k].idx];
		for(i=0;i<iinfo[k].len/2;i++){
			s[i*2+1]=*p++;
			s[i*2] = *p++;
		}
		s[i*2]=0;
		printf("%s: %s\n", iinfo[k].desc, s);
	}
	int capabilities = hdinfo[49];
	printf("LBA supported: %s\n", (capabilities & 0x0200)?"Yes":"No");
	int cmd_set_supported = hdinfo[83];
	printf("LBA48 supported: %s\n",(cmd_set_supported & 0x0400)?"Yes":"No");
	int sectors = ((int)hdinfo[61]<<16) + hdinfo[60];
	printf("HD size: %dMB\n", sectors * 512 / 1000000);
}

/**
 * output a command struct ptr.
 */
void hd_cmd_out(struct hd_cmd * cmd)
{
	if(!waitfor(STATUS_BSY, 0, HD_TIMEOUT)){
		panic("hd error.");
	}
	/* Activate the Interrupt Enable (nIEN) bit */
	_out_byte(REG_DEV_CTRL, 0);
	/* Load required parameters in the Command Block Registers */
	_out_byte(REG_FEATURES, cmd->features);
	_out_byte(REG_NSECTOR,  cmd->count);
	_out_byte(REG_LBA_LOW,  cmd->lba_low);
	_out_byte(REG_LBA_MID,  cmd->lba_mid);
	_out_byte(REG_LBA_HIGH, cmd->lba_high);
	_out_byte(REG_DEVICE,   cmd->device);
	/* Write the command code to the Command Register */
	_out_byte(REG_CMD,     cmd->command);		
}

/**
 * wait until a disk interrupt occurs
 */
void interrupt_wait()
{
	struct message msg;
	send_recv(RECEIVE, INTERRUPT, &msg);
}

/**
 * Wait for a certain status
 * @param mask	Status mask
 * @param val	Required status
 * timeout	Timeout in milliseconds
 */
int waitfor(int mask, int val, int timeout)
{
	int t = (int)get_ticks();
	while((((int)get_ticks()-t)*1000/HZ)<timeout){
		//
		if((_in_byte(REG_STATUS) & mask) == val){
			return 1;
		}
	}
	return 0;
}	

