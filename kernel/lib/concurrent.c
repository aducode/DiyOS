/**
 * 进程同步控制
 */
#include "type.h"
#include "syscall.h"
#include "clock.h"
/**
 * @function sleep
 * @brief 进程进sleep timeout 毫秒
 * @param timeout ms
 */
void sleep(long long timeout)
{
	if(timeout <= 10){
		//10ms发生一次clock 中断
		//sleep timeout 时间小于10ms，则直接返回
		return;
	}
	if(timeout <= 0L) return;
	timeout =(int) timeout * HZ / 1000; //10ms一次时钟中断，sleep基本单位是10ms
	struct message msg;
	reset_msg(&msg);
	msg.type = PSLEEP;
	msg.TIMEOUT = timeout;
	send_recv(BOTH, TASK_SYS, &msg);
}

