#include "proc.h"
#ifndef _DIYOS_STDLIB_H
#define _DIYOS_STDLIB_H
//系统调用
extern int send_recv(int function, int dest_src, struct message *msg);
extern int sendrec(int function, int dest_src, struct message *msg);
extern void printk(char *s);
//extern void sendrec(int function, int src_dest, int p_msg);
extern void write(char * buffer, int size);
#endif
