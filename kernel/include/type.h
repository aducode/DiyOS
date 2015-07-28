//定义一些全局类型
#ifndef _DIYOS_TYPE_H
#define _DIYOS_TYPE_H
/**
 * @define _SHOW_PROC_ENTRY_
 * @brief 用于调试，输出进程入口地址，不运行进程
 * @default off
 */
//#define _SHOW_PROC_ENTRY_
/**
 * @define _SHOW_PANIC_
 * @brief 用来判断是否打印系统级别panic信息
 * @default on
 */
#define _SHOW_PANIC_
#define _PANIC_N_	36
/**
 * @define _SHOW_MSG_SEND_
 * @brief 显示进程间发送的消息
 */
//#define _SHOW_MSG_SEND_
/**
 * @define _SHOW_MSG_RECEIVE_
 * @brief 显示进程接收到的消息
 */
//#define _SHOW_MSG_RECEIVE_

/**
 * @define _INCLUDE_KLIB_
 * @brief 是否include klib.h
 */
//#define _INCLUDE_KLIB_

//macro utils
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))



typedef unsigned long long	u64; //64bit长度
typedef unsigned int		u32; //32bit长度
typedef unsigned short		u16; //16bit长度
typedef unsigned char 		u8;  //8bit长度

typedef char* 			va_list;
typedef void* 			system_call;//系统调用


/**
 * 内部全部为整形的结构体
 */
struct mess1 {
	int m1i1;
	int m1i2;
	int m1i3;
	int m1i4;
};
/** 
 *指针message
 */
struct mess2{
	void* m2p1;
	void* m2p2;
	void* m2p3;
	void* m2p4;
};
/**
 *message类型3
 */
struct mess3{
	int m3i1;
	int m3i2;
	int m3i3;
	int m3i4;
	u64 m3l1;
	u64 m3l2;
	void* m3p1;
	void* m3p2;
};
/**
 * 定义ipc使用的message结构体
 */
struct message {
	int source;	//来自哪个线程
	int type;	//消息类型（以上定义了3种消息)
	union {
		struct mess1 m1;
		struct mess2 m2;
		struct mess3 m3;
	} u;	//消息体，根据type不同而不同
};

enum msgtype {
	HARD_INT = 1,		/*硬件中断*/
	//sys task
	GET_TICKS, 			/*获取当前系统ticket*/
	GET_PID,			/*获取当前进程PID*/
	//FS
	OPEN,				/*打开文件*/
	CLOSE,				/*关闭文件句柄*/
	READ,				/*读取文件内容*/
	WRITE,				/*写入文件*/
	SEEK,				/*更改文件指针位置*/
	TELL,				/*获取文件指针位置*/
	STAT, 				/*获取文件meta信息*/							/*未实现*/
	UNLINK,				/*删除文件*/
	MKDIR,				/*创建空目录，非递归*/
	RMDIR,				/*删除空目录(目录项含除了. ..之外项时报错)*/	/*未实现*/
	MOUNT,				/*挂载文件系统*/								/*未实现*/
	UNMOUNT,			/*卸载文件系统*/								/*未实现*/
	//FS & TTY
	SUSPEND_PROC,		/*挂起进程*/
	RESUME_PROC,		/*唤醒挂起的进程*/
	//MM
	FORK,				/*fork出子进程/fork文件句柄*/
	EXIT,				/*退出进程/减少子进程中与父进程共享的文件句柄的计数*/
	WAIT,				/*父进程等待子进程退出*/
	//TTY, SYS, FS, MM, etc
	SYSCALL_RET,		/*系统调用返回*/
	//message type for drivers
	DEV_OPEN = 1001,	/*打开硬件设备*/
	DEV_CLOSE,			/*关闭硬件设备*/
	DEV_READ,			/*从硬件中读取数据*/
	DEV_WRITE,			/*写入数据到硬件设备*/
	DEV_IOCTL			/*控制硬件设备*/
};
//macro for message
/**common return value**/
#define RETVAL u.m3.m3l1

//exit status
#define STATUS u.m3.m3i1

/**for message type DEV_IOCTL**/
#define REQUEST u.m3.m3i2

/**for message type DEV_READ or DEV_WRITE**/
#define CNT	u.m3.m3i2
#define PID	u.m3.m3i3
//which device
#define DEVICE	u.m3.m3i4
//read write position
#define POSITION u.m3.m3l1
//read write buffer
#define BUF	u.m3.m3p2


/**for FS message**/
#define FD		u.m3.m3i1
#define PATHNAME	u.m3.m3p1
#define FLAGS		u.m3.m3i1
#define NAME_LEN	u.m3.m3i2

#define OFFSET		u.m3.m3i2
#define WHERE		u.m3.m3i3

//about process
//#define MAX_PROCESS_COUNT     32      //最多32个进程
#define TASKS_COUNT             7       //系统进程个数
#define PROCS_COUNT             32      //系统支持最大进程数32
#define NATIVE_PROCS_COUNT      1       //用户进程数量

//默认优先级
//15:5时，可能死锁，所以先改成16:5
#define TASK_DEFAULT_PRIORITY   100      //系统进程权重
#define PROC_DEFAULT_PRIORITY	20	//用户进程权重

/**
 * @define TASKS
 * @brief define system task pid
 */
#define TASK_SYS        0
#define TASK_TTY        1
#define TASK_HD         2
#define TASK_FLOPPY	3
#define TASK_FS         4
#define TASK_MM         5

#define TASK_EMPTY      6

#define INIT            7

//进程权重
#define TASK_SYS_PRIORITY	TASK_DEFAULT_PRIORITY
#define TASK_TTY_PRIORITY	TASK_DEFAULT_PRIORITY
#define TASK_HD_PRIORITY	TASK_DEFAULT_PRIORITY
#define TASK_FLOPPY_PRIORITY	TASK_DEFAULT_PRIORITY
#define TASK_FS_PRIORITY	TASK_DEFAULT_PRIORITY
#define TASK_MM_PRIORITY	TASK_DEFAULT_PRIORITY

#define TASK_EMPTY_PRIORITY	1

#define INIT_PRIORITY		PROC_DEFAULT_PRIORITY

//消息广播
#define ANY     (TASKS_COUNT + PROCS_COUNT + 10)
//
#define NO_TASK (TASKS_COUNT + PROCS_COUNT + 20)
//has in msgtype
//#define HARD_INT                1
//中断类型消息
#define INTERRUPT               -10

#ifdef _INCLUDE_KLIB_
#include "klib.h"
#endif
#endif
