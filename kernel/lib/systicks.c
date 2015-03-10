#include "type.h"
#include "syscall.h"
#include "assert.h"
/**
 * 使用sendrec系统调用
 */
long long get_ticks()
{
        struct message msg;
        reset_msg(&msg);
        msg.type = GET_TICKS;
        send_recv(BOTH, TASK_SYS, &msg); //task_ticks被放在task table idx 2
	assert(msg.type == SYSCALL_RET);
        return msg.RETVAL;
}

