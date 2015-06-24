#include "type.h"
#include "syscall.h"
#include "assert.h"

/**
 * @function exit
 * @brief Terminate the current process.
 * 
 * @param status The value returned to the parent.
 *
 */
void exit(int status)
{
	struct message msg;
	reset_msg(&msg);
	msg.type = EXIT;
	msg.STATUS = status;
	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);
}

