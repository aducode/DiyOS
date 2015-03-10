#include "type.h"
#include "syscall.h"
#include "assert.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "stdio.h"

//#include "klib.h"
void task_sys()
{
	//_disp_str("ticks...",22,0,COLOR_YELLOW);
	struct message msg;
	while(1){
		send_recv(RECEIVE, ANY, &msg);//系统中断到内核级别，如果没有进程给这个进程发消息，那么就会阻塞
		int src = msg.source;
		switch(msg.type){
			case GET_TICKS:
				msg.RETVAL = ticks;
				msg.type = SYSCALL_RET;
				send_recv(SEND, src, &msg);
				break;
			case GET_PID:
				msg.type = SYSCALL_RET;
				msg.PID = src;
				send_recv(SEND, src, &msg);
				break;
			default:
				panic("unknown msg type");
				break;
		}
	}
}
