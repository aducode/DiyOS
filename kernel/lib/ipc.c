#include "type.h"
#include "klib.h"
#include "syscall.h"
#include "assert.h"
/*send_recv ipc
 * 暴露给ring1-3的系统调用是sendrec，这个函数对sendrec做了进一步封装
 */
int send_recv(int function, int src_dest, struct message *msg)  //0x4da0
{
//	printk("function:%d,src_dest:%d\n", function, src_dest);
        int ret = 0;
        if(function == RECEIVE){
                memset(msg, 0, sizeof(struct message));

        }
        switch(function){
                case BOTH:
                        ret = sendrec(SEND, src_dest, msg);
                        if(ret==0){
                                ret = sendrec(RECEIVE, src_dest, msg);
                        }
                        break;
                case SEND:
                case RECEIVE:
                        ret = sendrec(function, src_dest, msg);
                        break;
                default:
                        assert((function == BOTH) || (function == SEND) || (function == RECEIVE));
                        break;
        }
        return ret;
}

