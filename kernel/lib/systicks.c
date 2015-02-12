#include "type.h"
#include "syscall.h"
/**
 * 使用sendrec系统调用
 */
long long get_ticks()
{
        struct message msg;
        reset_msg(&msg);
        msg.type = GET_TICKS;
        send_recv(BOTH, 2, &msg); //task_ticks被放在task table idx 2
        return msg.RETVAL;
}

