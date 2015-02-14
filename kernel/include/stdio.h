#include "type.h"
#ifndef _DIYOS_STDIO_H
#define _DIYOS_STDIO_H
extern int open(const char * pathname, int flags);
extern int vsprintf(char *buffer, const char *fmt, va_list args);
extern int sprintf(char *buffer, const char* fmt, ...);
extern int printf(const char *fmt, ...);
#endif
