//定义进程表数据结构
#include "type.h"
#include "protect.h"
#ifndef _DIYOS_PROC_H
#define _DIYOS_PROC_H

extern void schedule();
//#define MAX_PROCESS_COUNT	32	//最多32个进程
#define TASKS_COUNT		4	//系统进程个数
#define PROCS_COUNT		0	//用户进程数量

#define MAX_PROCESS_COUNT	(TASKS_COUNT + PROCS_COUNT)	

//RPL LEVEL
#define RPL_TASK	1


//stacks of tasks
#define STACK_SIZE_TESTA	0x8000
#define STACK_SIZE_TOTAL	(STACK_SIZE_TESTA)
//进程表的栈，用来保存进程寄存器的值
struct stackframe{
	//中断开始时，由我们的中断处理函数进行压栈
	u32 gs;
	u32 fs;
	u32 es;
	u32 ds;
	u32 edi;
	u32 esi;
	u32 ebp;
	u32 kernel_esp;
	u32 ebx;
	u32 edx;
	u32 ecx;
	u32 eax;
	//
	u32 retaddr;
	//中断开始时，cpu会自动压栈
	u32 eip;
	u32 cs;
	u32 eflags;
	u32 esp;
	u32 ss;
};

//保存进程信息
struct process{
	struct stackframe regs; //进程寄存器的值
	
	u16 ldt_sel;		//ldt在gdt中的选择子
	struct descriptor ldts[MAX_LDT_ITEMS]; //local descriptor table
	int ticks;
	int priority;	//进程优先级
	
	u32 pid;	//进程id
	char p_name[16];	//进程名
};

typedef void (*proc_entry_point)();
//任务
struct task{
	proc_entry_point	entry;
	int stacksize;
	char name[32];	
};
#endif
