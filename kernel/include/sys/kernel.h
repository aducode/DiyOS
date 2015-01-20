#include "type.h"
#include "protect.h"
#include "global.h"	//定义了全局变量
#ifndef _KERNEL_H
#define _KERNEL_H
extern void _hlt();
extern void clean(int top, int left, int bottom, int right);
extern void disp_str(char * msg, int row, int column);
extern void _lgdt(unsigned short int limit_of_gdt, struct descriptor * base_of_gdt);
#endif
