#include "type.h"
#include "syscall.h"
#include "assert.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "stdio.h"
#include "clock.h"
#include "proc_queue.h"

/**
 * 节点超时
 */
static BOOL node_timeout(struct priority_queue_node * node);

void task_sys()
{
	init_queue(&sleep_queue);
	struct message msg;
	int ret, src, no_block;
	struct priority_queue_node new_node, node;
	while(1){
		no_block =  sleep_queue.size <= 0?0:NO_BLOCK;
		ret = send_recv(no_block|RECEIVE, ANY, &msg);//系统中断到内核级别，如果没有进程给这个进程发消息，那么就会阻塞
		if(ret==0){
			src = msg.source;
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
				case PSLEEP:
					//插入优先级队列
					//每10ms会发生一次时钟中断
					//
					new_node.run_tick = msg.TIMEOUT + ticks;
					new_node.pid = src;
					equeue(&sleep_queue, &new_node);
					break;
				default:
					panic("TASK_SYS unknown msg type:%d", msg.type);
					break;
			}
		} 
		while(dqueue_when(&sleep_queue, &node, node_timeout) == TRUE){
			//用户级进程
			reset_msg(&msg);
			msg.type = SYSCALL_RET;
			msg.RETVAL = 0;
			//唤醒sleep的进程
			send_recv(SEND, node.pid, &msg);
		}
	}
}


BOOL node_timeout(struct priority_queue_node * node)
{	
	//每10ms ticks增加1
	if(node->run_tick <= ticks) {
		return TRUE;
	} else {
		return FALSE;
	}
}
