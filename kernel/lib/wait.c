#include "type.h"
#include "syscall.h"
#include "assert.h"

/**
 * @function wait
 * @brief Wait for the child process to terminiate.
 * 
 * @param status The value returned from the child.
 *
 * @return PID of the terminated child.
 */
int wait(int *status)
{
	struct message msg;
	reset_msg(&msg);
	msg.type = WAIT;
	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);
	*status = msg.STATUS;
	return (msg.PID == NO_TASK?-1:msg.PID);
}
