#include "type.h"
#include "global.h"
#include "syscall.h"
#include "assert.h"
static void init_mm();
static int do_fork(struct message *msg);
/**
 * @function task_mm
 * @brief Main Loop for Memory Task
 */
void task_mm()
{
	init_mm();
	struct message msg;
	while(1){
		send_recv(RECEIVE, ANY, &msg);
		int src = msg.source;
		int reply = 1;
		switch(msg.type){
			case FORK:
				msg.RETVAL = do_fork(&msg);
				break;
			default:
				dump_msg("MM::unknown msg", &msg);
				assert(0);
				break;
		}
		if(reply){
			msg.type = SYSCALL_RET;
			send_recv(SEND, src, &msg);
		}
	}
}


/**
 * @function init_mm
 * @brief 初始化内存
 */
void init_mm()
{
//	printk("{MM} memsize:%dMB\n", boot_params.mem_size/(1024*1024));
	
}

/**
 * @function do_fork
 * @brief fork
 */
int do_fork(struct message *msg)
{
}
