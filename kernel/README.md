操作系统内核代码
* kernel.asm  内核入口的汇编代码
* head.c      用于重新设置gdt，初始化idt使用，供kernel.asm使用
* main.c      内核代码主函数，初始化多进程环境，启动多进程，初始化时钟中断响应函数，最终跳入进程体
* global.c    内核使用的全局变量，如一些系统级别的进程体
* proc.c      进程调度
