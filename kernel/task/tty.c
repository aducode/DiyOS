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
	for(p_tty = tty_table;p_tty<tty_table + CONSOLE_COUNT;p_tty++){
		init_tty(p_tty);
	}
	current_console = 0;
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
	int nr_tty = p_tty - tty_table;
	init_screen(p_tty);
}
/**
 * 转发到不同的tty
 * 供keyboard.c/keyboard_read(struct tty * p_tty)函数调用
 */
void dispatch_tty(struct tty *p_tty, u32 key){
	char output[2] = {'\0','\0'};
	if(!(key& FLAG_EXT)){
		if(p_tty->inbuf_count < TTY_IN_BYTES){
			*(p_tty->p_inbuf_head) = key;
			p_tty->p_inbuf_head ++;
			if(p_tty->p_inbuf_head == p_tty->in_buf+TTY_IN_BYTES){
				p_tty->p_inbuf_head = p_tty->in_buf;
			}
			p_tty -> inbuf_count++;
		} else {
			int raw_code = key & MASK_RAW;
			switch(raw_code){
				//功能按键
				case UP:
					/*if((key & FLAG_SHIFT_L)||(key & FLAG_SHIFT_R)){
						_disable_int();
						
					}
					*/
					break;
				case DOWN:
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
						//ALT + Fn
						_disp_str("ctrl+fn",0,0,COLOR_WHITE);
						select_console(raw_code - F1);
					}
				default:
					break;
			}	
		}
		//output[0] = key & 0xFF;
		//_disp_str(output,11,0,COLOR_WHITE);
	}	
}
