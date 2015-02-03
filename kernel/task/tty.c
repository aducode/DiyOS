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
static void tty_do_write(struct tty *p_tty);
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
	for(p_tty = tty_table+CONSOLE_COUNT-1;p_tty>=tty_table;p_tty--){
		init_tty(p_tty);
	}
	current_console = 0;
//	select_console(current_console);
	while(1)
	{
		for(p_tty = tty_table; p_tty < tty_table + CONSOLE_COUNT;p_tty++){
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}

void tty_do_read(struct tty * p_tty)
{
	if(is_current_console(p_tty->p_console)){
		keyboard_read(p_tty);
	}
}

void tty_do_write(struct tty *p_tty)
{
	if(p_tty->inbuf_count){
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail ++;
		if(p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES){
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count -- ;
		out_char(p_tty->p_console, ch);
	}
}
void init_tty(struct tty *p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;
	init_screen(p_tty);
}
/**
 * 转发到不同的tty
 * 供keyboard.c/keyboard_read(struct tty * p_tty)函数调用
 */
void tty_dispatch(struct tty *p_tty, u32 key){
	if(!(key& FLAG_EXT)){
		if(key != FLAG_CTRL_L && key != FLAG_CTRL_R && key != FLAG_ALT_L && key != FLAG_ALT_R && key != FLAG_SHIFT_L && key != FLAG_SHIFT_R){
			if(p_tty->inbuf_count < TTY_IN_BYTES){
				*(p_tty->p_inbuf_head) = key;
				p_tty->p_inbuf_head ++;
				if(p_tty->p_inbuf_head == p_tty->in_buf+TTY_IN_BYTES){
					p_tty->p_inbuf_head = p_tty->in_buf;
				}
				p_tty -> inbuf_count++;
			}
		}
	} else {
		int raw_code = key & MASK_RAW;
		switch(raw_code){
			//功能按键
			case UP:
				if((key & FLAG_SHIFT_L)||(key & FLAG_SHIFT_R)){
					scroll_screen(p_tty->p_console, SCR_DN);
					
				}
				break;
			case DOWN:
				if((key&FLAG_SHIFT_L)||(key&FLAG_SHIFT_R)){
					scroll_screen(p_tty->p_console, SCR_UP);
				}
				break;
			case F1:
			case F2:
			case F3:
			case F4:
			case F5:
			case F6:
			case F7:
			case F8:
			case F9:
			case F10:
			case F11:
			case F12:
				if((key&FLAG_CTRL_L)||(key&FLAG_CTRL_R)){
				//由于在我的linux中 alt+fn 有其他的操作，所以用ctrl+fn测试，通过了
				//if((key & FLAG_ALT_L) || (key & FLAG_ALT_R )){
					//ALT + Fn
					select_console(raw_code - F1);
				}
			default:
				break;
		}	
	}	
}
