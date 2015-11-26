/**
 * 进程同步控制
 */
#include "type.h"
#include "syscall.h"

/**
 * @function sleep
 * @brief 进程进sleep timeout 毫秒
 * @param timeout ms
 */
void sleep(long long timeout)
{
	if(timeout <= 0L) return;
	struct message msg;
	reset_msg(&msg);
	msg.type = PSLEEP;
	msg.TIMEOUT = timeout;
	send_recv(BOTH, TASK_SYS, &msg);
}

