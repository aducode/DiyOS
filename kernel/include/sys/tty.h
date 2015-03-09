#include "type.h"
#include "console.h"
#ifndef _DIYOS_TTY_H
#define _DIYOS_TTY_H

/**
 *输入缓冲区大小,这是键盘输入的字符缓冲，不是键盘硬件扫描码缓冲（扫描码缓冲在keyboard.c中，供硬件使用)
 */
#define TTY_IN_BYTES	256
#define TTY_OUT_BUF_LEN	2	//tty output buffer size 
/**
 * 终端个数
 */
#define CONSOLE_COUNT	3
/**
 *tty
 */
struct tty{
	u32	in_buf[TTY_IN_BYTES];	//tty输入缓冲区
	u32*    p_inbuf_head;		//指向缓冲区下一个空闲位置
	u32*	p_inbuf_tail;		//指向键盘任务应该处理的键值
	int	inbuf_count;		//缓冲区已经填充了多少
	//add new field
	int tty_caller;			//用来保存向tty发送消息的进程（一般情况下是TASK_FS)
	int tty_req_pid;		//用来保存请求数据的进程id
	void *tty_req_buf;		//用户进程缓冲区地址
	int tty_left_count;		//保存用户请求字符数
	int tty_trans_count;		//已经向用户进程传送了多少字符
	//
	struct console* p_console;	//记录光标位置等控制台信息
};
extern void tty_dispatch(struct tty *p_tty, u32 key);


#endif
