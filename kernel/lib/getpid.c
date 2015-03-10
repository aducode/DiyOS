#include "type.h"
#include "syscall.h"
#include "assert.h"
/**
 * @function getpid
 * @brief 获取当前进程id
 */
int getpid()
{
	struct message msg;
	msg.type = GET_PID;
	send_recv(BOTH, TASK_SYS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.PID;
}
