#include "type.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "keyboard.h"
#include "global.h"
#include "assert.h"
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
 * 放入tty的缓冲区
 */
static void put_key(struct tty *p_tty, u32 key);
/**
 * 向tty中的console输出
 */
static void tty_write(struct tty *p_tty, char *buffer, int size);
/**
 * tty进程体
 * Main Loop of TTY task
 */
void task_tty()
{
//	panic("test");
//	assert(0);
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

void put_key(struct tty *p_tty, u32 key){
	if(p_tty->inbuf_count < TTY_IN_BYTES){
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if(p_tty->p_inbuf_head == p_tty->in_buf+TTY_IN_BYTES){
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}
/**
 * 转发到不同的tty
 * 供keyboard.c/keyboard_read(struct tty * p_tty)函数调用
 */
void tty_dispatch(struct tty *p_tty, u32 key){
	if(!(key& FLAG_EXT)){
		if(key != FLAG_CTRL_L && key != FLAG_CTRL_R && key != FLAG_ALT_L && key != FLAG_ALT_R && key != FLAG_SHIFT_L && key != FLAG_SHIFT_R){
			put_key(p_tty, key);
		}
	} else {
		int raw_code = key & MASK_RAW;
		switch(raw_code){
			//功能按键
			case ENTER:
				put_key(p_tty, '\n');
				break;
			case BACKSPACE:
				put_key(p_tty, '\b');
				break;
			case UP:
				if((key & FLAG_SHIFT_L)||(key & FLAG_SHIFT_R)){
					scroll_screen(p_tty->p_console, SCR_UP);
					
				}
				break;
			case DOWN:
				if((key&FLAG_SHIFT_L)||(key&FLAG_SHIFT_R)){
					scroll_screen(p_tty->p_console, SCR_DN);
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

/****************************************************************************************************************/
void tty_write(struct tty *p_tty, char *buffer, int size){
	char *p = buffer;
	int i = size;
	while(i){
		out_char(p_tty->p_console, *p++);
		i--;
	}
}


/**
 *系统调用
 * 现在系统调用使用了4个参数，这个地方_unsed1存储ebx的值，但是write没有使用ebx，这里
 * 忽略
 */
int sys_write(int _unsed1, char *buffer,int size, struct process *p_proc)
{
	tty_write(&tty_table[p_proc->tty_idx], buffer, size);
	return 0;
}
/**
 *系统调用，内核级打印
 */
int sys_printk(int _unsed1, int _unsed2, char *s, struct process *p_proc)
{
	const char * p;
	char ch;
	char reenter_err [] = "? k_reenter is incorrect for unknown reason";
	reenter_err[0] = MAG_CH_PANIC;
	if(k_reenter == 0) {
		//ring1 --- ring3调用
		//需要将虚拟地址转为线性地址
		p = va2la(proc2pid(p_proc), s);	
		//p=s;
	} else if(k_reenter>0){
		//ring0调用
		p=s;
	} else {
		p=reenter_err;
	}
	//通过字符串第一个字符判断错误级别
	if((*p == MAG_CH_PANIC)||(*p== MAG_CH_ASSERT && p_proc_ready < &proc_table[TASKS_COUNT])){
		_disable_int();
		char *v = (char*)V_MEM_BASE;
		const char * q = p+1;	//skip
		while(v<(char*)(V_MEM_BASE+V_MEM_SIZE)){
			*v++ = *q++;
			*v++ = RED_CHAR;
			if(!*q){
				while(((int)v - V_MEM_BASE)%(SCR_WIDTH*16)){
					v++;
					*v++ = GRAY_CHAR;
				}
				q = p+1;
			}
		}
		__asm__ __volatile__("hlt");
	}
	while((ch=*p++)!=0){
		if(ch==MAG_CH_PANIC ||ch==MAG_CH_ASSERT) continue;
		out_char(tty_table[p_proc->tty_idx].p_console, ch);
	}
	return 0;
}
