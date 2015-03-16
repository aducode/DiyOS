//定义进程表数据结构
#include "type.h"
#include "protect.h"
#include "fs.h"
#ifndef _DIYOS_PROC_H
#define _DIYOS_PROC_H

#define PROCS_BASE		0x300000 //3M 3M以上的空间留给用户进程使用
					 //2M的空间是系统文件缓冲区
#define PROC_IMAGE_SIZE_DEFAULT	0x100000 //默认进程空间大小1M
#define PROC_ORIGIN_STACK	0x400	 //1KB
//
/**
 * @define INVALID_DRIVER
 * @brief define the invalid driver pid
 */
#define INVALID_DRIVER  -20
//进程栈
//stacks of tasks
#define STACK_SIZE_TASK_TTY	0x8000
#define STACK_SIZE_TASK_HD	0x8000
#define STACK_SIZE_TASK_SYS	0x8000
#define STACK_SIZE_TASK_FS	0x8000
#define STACK_SIZE_TASK_MM	0x8000
//由于empty_proc只是一个空进程体，所以不需要使用栈
#define STACK_SIZE_TASK_EMPTY	0x0
//stacks of process
#define STACK_SIZE_INIT	0x8000
//#define STACK_SIZE_PROC_TESTA	0x8000
/*
#define STACK_SIZE_PROC_TESTB	0x8000
#define STACK_SIZE_PROC_TESTC	0x8000
#define STACK_SIZE_PROC_TESTD	0x8000
*/
//stacks size
#define STACK_SIZE_TOTAL        (STACK_SIZE_TASK_TTY + \
				 STACK_SIZE_TASK_HD + \
				 STACK_SIZE_TASK_SYS + \
				 STACK_SIZE_TASK_FS + \
				 STACK_SIZE_TASK_MM + \
				 STACK_SIZE_TASK_EMPTY + \
				 STACK_SIZE_INIT)
/* + \
				 STACK_SIZE_PROC_TESTB + \
				 STACK_SIZE_PROC_TESTC + \
				 STACK_SIZE_PROC_TESTD)
*/

//任务状态
#define SENDING			0x02	/*set when proc trying to send*/
#define RECEIVING		0x04	/*set when proc trying to recv*/
#define WAITING			0x08	/*set when proc waiting for the child to terminate*/
#define HANGING			0x10	/*set when proc exits without being waited by parent*/
#define FREE_SLOT		0x20	/*set when proc table entry is not used*/
//这里暂时将PROCS_COUNT写死，所以MAX_PROCESS_COUNT就为tasks + procs
#define MAX_PROCESS_COUNT	(TASKS_COUNT + PROCS_COUNT)	

//RPL LEVEL
#define RPL_TASK	1
#define RPL_USER	3


//fs
/**
 * @def MAX_FILE_COUNT
 * @brief 每个进程可以打开的最大文件数
 */
#define MAX_FILE_COUNT	64
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
	char name[16];	//进程名
	int p_flags;		//进程标志 SENDING正在发消息状态阻塞 RECEIVING正在接收消息状态阻塞  0分配时间片正在运行或准备运行
	struct message *p_msg;	//存储进程接受或发送的消息
	int p_recvfrom;		//消息来源的pid
	int p_sendto;		//消息发送的目的pid
	int has_int_msg;	//如果非0，说明正在处理中断
	struct process *q_sending;	//向该进程发送消息的队列
	struct process *next_sending;	//队列头
	//int tty_idx;	//tty表索引
	//fs
	int p_parent;			//pid of parent process
	int exit_status;		//for parent
	struct file_desc *filp[MAX_FILE_COUNT];   //这里保存的是struct file_desc 指针类型，真正的struct file_desc作为全局变量单独保存在global.c中的f_desc_table中
};

typedef void (*proc_entry_point)();
//任务
struct task{
	proc_entry_point	entry;
	int stacksize;
	char name[32];	
};


#define proc2pid(x)     (x-proc_table)
extern void schedule();
extern void* va2la(int pid, void* va);
extern void reset_msg(struct message *p_msg);
//extern void block(struct process *p_proc);
//extern void unblock(struct process *p_proc);
//extern msg_send(struct process *current, int dest,struct message *m);
//extern msg_receive(struct process *current, int src, struct message *m);
/**
 * 发送接收消息的系统调用
 * @param function SEND RECEIVE BOTH
 * @param src_dest  消息的目标或者来源
 * @param msg 消息体指针
 */
extern int send_recv(int function, int src_dest, struct message *msg);
extern void dump_msg(const char * title, struct message *msg);
extern void inform_int(int task_idx);
//
extern void interrupt_wait();
extern int waitfor(int reg_port, int mask, int val, int timeout);
#endif
