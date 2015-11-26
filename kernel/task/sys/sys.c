#include "type.h"
#include "syscall.h"
#include "assert.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "stdio.h"
#include "clock.h"

static void spinning();

/**
 * 超时时间为权重的优先队列
 */
static struct priority_queue_elem {
	long long next_ticks;	//需要运行的ticks
	int pid;		//pid
} priority_queue[PROCS_COUNT];
static int queue_size;
static int queue_head, queue_tail;
/**
 * @function equeue
 * @brief 插入优先级队列
 * 
 * @param timtoue
 * @param pid
 * 
 * @return 0 success
 */
static int equeue(long long timeout, int pid);

/**
 * @function dqueue
 * @brief 取出pid
 * 
 * @param out pid
 * 
 * @return 0 success
 */
static int dqueue(int * pid);

/**
 * 满足条件的时候出队列
 */
static int check_and_dequeue(int *pid);

void task_sys()
{
	queue_size = queue_head = 0;
	queue_tail = 1;
	struct message msg;
	int ret, src, no_block;
	int interrupt_pid;
	while(1){
		no_block =  queue_size == 0?0:NO_BLOCK;
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
					printk("PID:%d is sleeping..%dms\n", msg.source, msg.TIMEOUT);
					equeue(msg.TIMEOUT, src);	
					break;
				default:
					panic("TASK_SYS unknown msg type:%d", msg.type);
					break;
			}
		} 
		//else  {
			//spinning();
		//}
		//异步处理
		//lock_kernel();
		while((ret=check_and_dqueue(&interrupt_pid)) == 0){
			//用户级进程
			assert(interrupt_pid>=TASKS_COUNT && interrupt_pid < MAX_PROCESS_COUNT);
			reset_msg(&msg);
			msg.type = SYSCALL_RET;
			msg.RETVAL = 0;
			//唤醒sleep的进程
			send_recv(SEND, interrupt_pid, &msg);
		}
		//unlock_kernel();
	}
}

void spinning()
{
	long long _ticks = ticks;
	while((ticks - _ticks)*1000/HZ < 15);
}

int equeue(long long timeout, int pid)
{
	long long _ticks = ((int)timeout * HZ) + ticks * 1000;
	int i = queue_head;
	while(i<queue_tail){
		i += 1;
		if(i>=PROCS_COUNT){
			i=0;
		}
		//if(
	}
	for(i=queue_head;i<queue_tail;i++){
		if(priority_queue[i].next_ticks > _ticks){
			break;
		}
	}
	queue_size ++;
	return -1;
}


int dqueue(int * pid)
{
	return -1;
}

int check_and_dqueue(int *pid)
{
	return -1;
}
