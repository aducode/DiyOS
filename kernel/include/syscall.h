#include "type.h"
#ifndef _DIYOS_SYSCALL_H
#define _DIYOS_SYSCALL_H
//系统调用
//send_recv使用的宏
//function 参数
#define SEND		1
#define RECEIVE		2
#define BOTH		3
//NO BLOCK标志位
#define NO_BLOCK	0x80
/**
 * @function send_recv
 * @brief 进程间通信
 * @param function SEND RECEIVE 
 * @param dest_src 目标进程PID
 * @param msg 消息体
 * @return
 */
extern int send_recv(int function, int dest_src, struct message *msg);
/**
 * @function sendrecv
 * @brief 进程间通信，封装send_recv，同时处理BOTH(SEND & RECEIVE)
 * @param function SEND RECEIVE BOTH
 * @param dest_src 目标进程PID
 * @param msg 消息体
 * @return
 */
extern int sendrec(int function, int dest_src, struct message *msg);
/**
 * @function printk0
 * @brief 内核级的标准输出
 * @param s
 * @return
 */
extern void printk0(char *s);
//extern void sendrec(int function, int src_dest, int p_msg);
//extern void write0(char * buffer, int size);
#endif
