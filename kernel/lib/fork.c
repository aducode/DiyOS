#include "type.h"
#include "syscall.h"
#include "assert.h"

/**
 * @function fork
 * @brief Create a child process, which is actually a copy of the caller
 * @return On success, the PID of the child process is returned in the parent's thread of execution, and a 0 is returned in the child's thread of execution.
 *  On failure, a -1 will be returned in the parent's context, no child process will be created.
 */
int fork()
{
	struct message msg;
	reset_msg(&msg);
	msg.type = FORK;
	
	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);
	assert(msg.RETVAL == 0);
	return msg.PID;
}
