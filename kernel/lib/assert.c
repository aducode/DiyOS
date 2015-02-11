#include "assert.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
void spin(const char *func_name)
{
	printf("\nspinning in %s ...\n",func_name);
	while(1){}
}
/**
 * 打印错误信息，并且停止进程（或系统）
 */
void assertion_failure(char *exp, char *file, char *base_file, int line)
{
	char buffer[256];
	sprintf(buffer,"%c assert(%s) failed: file: %s, base_file: %s, line %d",		MAG_CH_ASSERT,
		exp, file, base_file, line);
	//syscall printk
	printk(buffer);	
	//如果没有返回，说明已经在内核级hlt
	//如果返回了，则停止线程
	spin("assertion_failure()");
	//should never arrive here
	__asm__ __volatile__("ud2");	
}

/**
 * 系统内核级错误 显示并hlt系统
 */
void panic(const char *fmt,...)
{
	char buf[256];
	char buf2[256];
	va_list arg = (va_list)((char*)&fmt+4);
	vsprintf(buf, fmt, arg);
	sprintf(buf2,"%c !! panic!! %s",MAG_CH_PANIC, buf);
	printk(buf2);
	//should never arrive here
	__asm__ __volatile__("ud2");
}
