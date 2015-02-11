#include "type.h"
#ifndef _DIYOS_STDIO_H
#define _DIYOS_STDIO_H
/**
 * 发送接收消息的系统调用
 * @param function SEND RECEIVE BOTH
 * @param src_dest  消息的目标或者来源
 * @param msg 消息体指针
 */
extern send_recv(int function, int src_dest, struct message *msg);

extern int vsprintf(char *buffer, const char *fmt, va_list args);
extern int sprintf(char *buffer, const char* fmt, ...);
extern int printf(const char *fmt, ...);
#endif
