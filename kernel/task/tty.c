#include "type.h"
#include "proc.h"
#include "syscall.h"
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
static void tty_do_read(struct tty * p_tty, struct message *p_msg);

/**
 * 从输入设备中读取字符串（键盘）
 */ 
static void tty_dev_read(struct tty *p_tty);
/**
 * 输出字符到console
 */
static void tty_do_write(struct tty *p_tty, struct message *p_msg);

/**
 * 将字符写入到输出设备（显示器）
 */
static void tty_dev_write(struct tty *p_tty);
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
	struct message msg;
	
	init_keyboard();
	struct tty *p_tty;
	for(p_tty = tty_table+CONSOLE_COUNT-1;p_tty>=tty_table;p_tty--){
		init_tty(p_tty);
	}
//	current_console = 0;
//	select_console(current_console);
	select_console(0);
	while(1)
	{
		for(p_tty = tty_table; p_tty < tty_table + CONSOLE_COUNT;p_tty++){
			do{
				//tty_do_read(p_tty);
				//tty_do_write(p_tty);
				tty_dev_read(p_tty);	//键盘中的数据写入缓冲区
				tty_dev_write(p_tty);	//缓冲区中的数据输出到显示器
			}while(p_tty->inbuf_count);
		}
		//上面处理键盘输入输出
		//下面处理其他进程将tty作为文件操作时的操作
		send_recv(RECEIVE, ANY , &msg);
		int src = msg.source;
		assert(src != TASK_TTY);
		struct tty *ptty = &tty_table[msg.DEVICE];
		switch(msg.type){
			case DEV_OPEN:
				reset_msg(&msg);
				msg.type = SYSCALL_RET;		//
				send_recv(SEND, src, &msg);
				break;
			case DEV_READ:
				tty_do_read(ptty, &msg);
				break;
			case DEV_WRITE:
				tty_do_write(ptty, &msg);
				break;
			case HARD_INT:
				//wake up by clock_handler -- a key was just pressed
				//@see clock_handler() inform_int()
				key_pressed = 0;
				continue;
			default:
				dump_msg("TTY:unknow msg", &msg);
				break;
		}
	}
}
/**
 * @function tty_dev_read
 * @brief 将键盘中的字符串放到tty中的输入缓冲中
 * @param p_tty tty ptr
 * 
 */
void tty_dev_read(struct tty * p_tty)
{
	if(is_current_console(p_tty->p_console)){
		keyboard_read(p_tty);
	}
}

/**
 * @function tty_do_read
 * @brief 处理tty被其他进程当作文件时的读操作
 *        操作系统中由FS进程操作tty进程，所以还好保存哪个进程调用了FS进程进行tty的读写，最终结果由tty传回到保存的pid
 * @param p_tty tty ptr From which TTY the caller proc wants to read
 * @param p_msg message ptr The MESSAGE just received
 *
 */
void tty_do_read(struct tty * p_tty, struct message *p_msg)
{
	//tell the tty:
	p_tty->tty_caller = p_msg->source;	//who called, usually FS
	p_tty->tty_req_pid = p_msg->PID;		//who wants the chars
	p_tty->tty_req_buf = va2la(p_tty->tty_req_pid, p_msg->BUF); //where the chars should be put
	p_tty->tty_left_count = p_msg->CNT;	//how many chars are requested
	p_tty->tty_trans_count = 0;		//how many chars have been transferred
	
	p_msg->type = SUSPEND_PROC;
	p_msg->CNT  = p_tty->tty_left_count;
	send_recv(SEND, p_tty->tty_caller, p_msg);
}


/**
 * @function tty_dev_write
 * @brief 将tty的输入缓冲区中的数据写入显示器
 * @param p_tty tty ptr
 * 
 */
void tty_dev_write(struct tty *p_tty)
{
	while(p_tty->inbuf_count){
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail ++;
		if(p_tty->p_inbuf_tail ==  p_tty->in_buf + TTY_IN_BYTES){
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count --;
		if(p_tty->tty_left_count){
			//如果用户进程想读取多个字符
			if(ch >= ' ' && ch <= '~'){
				//可打印字符
				out_char(p_tty->p_console, ch);
				void *p = p_tty->tty_req_buf + p_tty->tty_trans_count;
				memcpy(p,(void*)va2la(TASK_TTY, &ch), 1);
				p_tty->tty_trans_count ++;
				p_tty->tty_left_count --;
			} else if (ch == '\b' && p_tty->tty_trans_count){
				//退格
				out_char(p_tty->p_console, ch);
				p_tty->tty_trans_count --;
				p_tty->tty_left_count ++;
			} else if(ch == '\n'/* || p_tty->tty_left_count == 0*/){
				//如果遇到换行 或  left count为0
				out_char(p_tty->p_console, '\n');
				struct message msg;
				msg.type = RESUME_PROC;
				msg.PID = p_tty->tty_req_pid;
				msg.CNT = p_tty->tty_trans_count;
				send_recv(SEND, p_tty->tty_caller, &msg);
				p_tty->tty_left_count = 0;
			}
		}
	}
	/*
	if(p_tty->inbuf_count){
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail ++;
		if(p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES){
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count -- ;
		out_char(p_tty->p_console, ch);
	}
	*/
}

/**
 * @function tty_do_write
 * @brief 处理tty被其他进程作为文件时的写操作
 * @param p_tty tty ptr
 * @param p_msg message ptr
 *
 */
void tty_do_write(struct tty *p_tty, struct message * p_msg)
{
	char buf[TTY_OUT_BUF_LEN];
	char *p = (char*)va2la(p_msg->PID, p_msg->BUF);
	int i = p_msg->CNT;
	int j;
	while(i){
		int bytes = min(TTY_OUT_BUF_LEN, i);
		memcpy(va2la(TASK_TTY, buf), (void*)p, bytes);
		for(j=0;j<bytes;j++){
			out_char(p_tty->p_console, buf[j]);
		}
		i -= bytes;
		p += bytes;
	}
	p_msg->type = SYSCALL_RET;
	send_recv(SEND, p_msg->source, p_msg);
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
