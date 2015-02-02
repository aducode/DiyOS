#include "type.h"
#include "tty.h"
#include "console.h"
#include "keyboard.h"
#include "klib.h"
#include "global.h"
//全局变量
/**
 *tty和console表
 */
struct tty tty_table[CONSOLE_COUNT];
struct console console_table[CONSOLE_COUNT];
//静态函数
/**
 * 读字符串
 */
static void tty_do_read(struct tty * p_tty);
/**
 * 输出字符到console
 */
static void tty_do_write();
/**
 * 初始化tty
 */
static void init_tty(struct tty *p_tty);
/**
 * tty进程体
 */
void task_tty()
{
	//进程开始时先初始化键盘
	init_keyboard();
	struct tty *p_tty;
	for(p_tty = tty_table;p_tty<tty_table + CONSOLE_COUNT;p_tty++){
		init_tty(p_tty);
	}
	current_console = 0;
	while(1)
	{
		keyboard_read(p_tty);
	}
}

void init_tty(struct tty *p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;
}
/**
 * 转发到不同的tty
 * 供keyboard.c/keyboard_read(struct tty * p_tty)函数调用
 */
void dispatch_tty(struct tty *p_tty, u32 key){
	char output[2] = {'\0','\0'};
	if(!(key& FLAG_EXT)){
		output[0] = key & 0xFF;
		_disp_str(output,11,0,COLOR_WHITE);
	}	
}
