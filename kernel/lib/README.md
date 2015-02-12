对内核中的函数做一些封装
* klib.asm	定义一些c语言不能实现的汇编函数
* string.c	字符串 / 内存相关的函数
* stdio.c	模仿printf方法，供ring3级别的用户进程使用，将字符串输出到进程对应的tty的console中，最终显示在屏幕上,printf 目前支持%d %x %o %s格式化输出,需要include "stdio.h"头文件
* ipc.c		进程通讯
* assert.c	assert和panic
* systicks.c	获取系统时间
* syscall.asm	系统调用函数 
