对内核中的函数做一些封装

至于函数按什么逻辑分到不同的lib文件下，就不要追究了，都是脑子一热就写的。。。

* klib.asm	定义一些c语言不能实现的汇编函数
* string.c	字符串 / 内存相关的函数
* stdio.c	模仿printf方法，供ring3级别的用户进程使用，将字符串输出到进程对应的tty的console中，最终显示在屏幕上,printf 目前支持%d %x %o %s格式化输出,需要include "stdio.h"头文件
* stat.c	提供stat函数，获取文件属性  mkdir函数，创建目录  rmdir函数，删除空目录
* mount.c	挂载和卸载设备
* fork.c	fork函数
* exit		exit函数
* wait		wait函数
* ipc.c		进程通讯
* assert.c	assert和panic
* systicks.c	获取系统时间
* syscall.asm	系统调用函数 
