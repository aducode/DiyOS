#include "type.h"
#include "protect.h"
#include "proc.h"
#include "global.h"	//定义了全局变量
#include "clock.h"
#include "string.h"
#include "assert.h"
//#include "keyboard.h"

//#include "string.h"
#include "klib.h"

//#include "stdlib.h"
#ifndef _DIYOS_KERNEL_H
#define _DIYOS_KERNEL_H
extern void _restart();
extern void _sys_call();

//head.c
extern void init_desc(struct descriptor * p_desc, u32 base, u32 limit, u16 attribute);
#endif
