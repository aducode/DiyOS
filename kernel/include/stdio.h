#include "type.h"
#ifndef _DIYOS_STDIO_H
#define _DIYOS_STDIO_H
/**
 * @def MAX_PATh
 * @brief 最大文件路径长度
 */
#define MAX_PATH	128

/**
 * @def O_CREATE & O_RDWT
 * @brief open flags参数取值
 */
#define O_CREATE	1
#define O_RDWT		2

extern int open(const char * pathname, int flags);
extern int close(int fd);
extern int read(int fd, void *buf, int count);
extern int write(int fd, const void * buf, int count);
extern int vsprintf(char *buffer, const char *fmt, va_list args);
extern int sprintf(char *buffer, const char* fmt, ...);
extern int printf(const char *fmt, ...);
#endif
