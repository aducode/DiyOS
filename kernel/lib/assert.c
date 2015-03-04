#include "assert.h"
#include "syscall.h"
#include "stdio.h"
#include "string.h"
/**
 * @function prink
 * @brief 对系统调用printk0的封装，供内核使用
 */
int printk(const char*fmt, ...)
{
       int i;
        char buf[256];
        va_list arg = (va_list)((char*)(&fmt)+4);
       
        i = vsprintf(buf,fmt, arg);
        printk0(buf);
        return i;

}

void spin(const char *func_name)
{
	printk("\nspinning in %s ...\n",func_name);
	while(1){}
}
/**
 * 打印错误信息，并且停止进程（或系统）
 */
void assertion_failure(char *exp, char *file, char *base_file, int line)
{
	//syscall printk
	printk("%c assert(%s) failed: file:%s, base_file:%s, line %d",	MAG_CH_ASSERT, exp, file, base_file, line);	
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
	va_list arg = (va_list)((char*)&fmt+4);
	vsprintf(buf, fmt, arg);
	printk("%c !! panic!! %s", MAG_CH_PANIC, buf);
	//should never arrive here
	__asm__ __volatile__("ud2");
}
