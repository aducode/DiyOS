#include "type.h"
#include "protect.h"
#include "proc.h"
#include "global.h"	//定义了全局变量

#include "string.h"
#ifndef _DIYOS_KERNEL_H
#define _DIYOS_KERNEL_H
//klib.asm中的汇编函数
extern void _hlt();
extern void _clean(int top, int left, int bottom, int right);

//color
#define COLOR_GREEN		0x02
#define COLOR_LIGHT_WHITE	0x07
#define	COLOR_WHITE		0x0F
#define COLOR_SYS_ERROR		0x74
extern void _disp_str(char * msg, int row, int column, u8 color);
extern void * _memcpy(void * dest, void * src, int size);
extern void _memset(void * dest, char ch, int size);
extern void _out_byte(u16 port, u8 value);
extern u8 _in_byte(u16 port);
//kernel.asm
extern void _restart();


extern void _disable_irq(int irq_no);
extern void _enable_irq(int irq_no);
#endif
