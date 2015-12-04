/*******************************************************/
/*                    软盘驱动                         */
/*******************************************************/
#include "type.h"
#include "syscall.h"
#include "proc.h"
#include "assert.h"
#include "floppy.h"
void task_floppy()
{
	struct message msg;//用于进程间通信
	do_init_floppy();
	while(1)
	{
		//floppy driver main loop
		send_recv(RECEIVE, ANY, &msg); //接收广播
		int src = msg.source;
		switch(msg.type){
			case DEV_OPEN:
				do_floppy_open(msg.DEVICE);
				msg.type=SYSCALL_RET;
				break;
			case DEV_CLOSE:
				do_floppy_close(msg.DEVICE);
				break;
			case DEV_READ:
			case DEV_WRITE:
				do_floppy_rdwt(&msg);
				break;
			case DEV_IOCTL:
				do_floppy_ioctl(&msg);
				break;
			default:
				dump_msg("FLOPPY driver::unknown msg", &msg);
				spin("FLOPPY::main_loop (invalid msg.type)");
				break;
		}
		send_recv(SEND, src, &msg);
	}
}
