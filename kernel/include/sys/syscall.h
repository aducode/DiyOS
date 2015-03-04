#include "type.h"
#ifndef _DIYOS_SYSCALL_H
#define _DIYOS_SYSCALL_H
//系统调用
//send_recv使用的宏
//function 参数
#define SEND		1
#define RECEIVE		2
#define BOTH		3
//dest_src
#define INVALID_DRIVER	-20
#define TASK_TTY	0
#define TASK_HD		1
#define TASK_TICKS	2
#define TASK_FS		3
extern int send_recv(int function, int dest_src, struct message *msg);
extern int sendrec(int function, int dest_src, struct message *msg);
extern void printk0(char *s);
//extern void sendrec(int function, int src_dest, int p_msg);
extern void write0(char * buffer, int size);
#endif
