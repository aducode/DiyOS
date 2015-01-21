#include "type.h"
#include "protect.h"
#include "global.h"	//定义了全局变量
#ifndef _DIYOS_KERNEL_H
#define _DIYOS_KERNEL_H
//kernel.asm中的汇编函数
extern void _hlt();
extern void _clean(int top, int left, int bottom, int right);
extern void _disp_str(char * msg, int row, int column);
extern void * _memcpy(void * dest, void * src, int size);
extern void _out_byte(u16 port, u8 value);
extern u8 _in_byte(u16 port);
#endif
