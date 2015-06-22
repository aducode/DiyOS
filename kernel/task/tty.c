#include "type.h"
#include "syscall.h"
#include "proc.h"
#include "syscall.h"
#include "tty.h"
#include "console.h"
#include "keyboard.h"
#include "global.h"
#include "assert.h"

//#include "klib.h"
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
static void tty_dev_read(struct tty * p_tty);
/**
 * 输出字符到console
 */
static void tty_dev_write(struct tty *p_tty);

/**
 * @function tty_do_read
 * @brief tty进程收到读数据消息后调用
 * @param tty  tty指针
 * @param message msg指针
 */
void tty_do_read(struct tty * tty, struct message * msg);
/**
 * @function tty_do_write
 * @brief tty进程收到写消息后调用
 * @param tty tty指针
 * @param message msg指针
 */
void tty_do_write(struct tty *tty, struct message *msg);
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
//	_disp_str("tty.",20,0,COLOR_WHITE);
	struct message msg;
	//进程开始时先初始化键盘
	init_keyboard();
	struct tty *p_tty;
	for(p_tty = tty_table+CONSOLE_COUNT-1;p_tty>=tty_table;p_tty--){
		init_tty(p_tty);
	}
//	current_console = 0;
	select_console(0);
	while(1)
	{
		for(p_tty = tty_table; p_tty < tty_table + CONSOLE_COUNT;p_tty++){
			tty_dev_read(p_tty);
			tty_dev_write(p_tty);
		}
		//这里接收其广播的消息
		//如果没有其他进程发送，则会阻塞在此
		send_recv(RECEIVE, ANY, &msg);
		int src = msg.source;
		assert(src != TASK_TTY);
		struct tty *p_tty = &tty_table[msg.DEVICE];
		switch(msg.type){
			case DEV_OPEN:
				reset_msg(&msg);
				msg.type = SYSCALL_RET;
				send_recv(SEND, src, &msg);
				break;
			case DEV_READ:
				tty_do_read(p_tty, &msg);
				break;
			case DEV_WRITE:
				tty_do_write(p_tty, &msg);
				break;
			case HARD_INT:
				//waked up by clock_handler
				key_pressed = 0;
				continue;
			defualt:
				dump_msg("TTY:unknown msg", &msg);
				break;
		}
	}
}
void tty_dev_read(struct tty * p_tty)
{
	if(is_current_console(p_tty->p_console)){
		keyboard_read(p_tty);
	}
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
}
//用户进程读取从键盘输入的数据后调用这个函数
void tty_do_read(struct tty * tty, struct message *msg)
{
	tty->tty_caller = msg->source; //谁调用tty的read ，一般是TASK_FS进程
	tty->tty_req_pid =  msg->PID;  //真正请求数据的用户进程
	tty->tty_req_buf = va2la(tty->tty_req_pid, msg->BUF); //用户进程缓冲区
	tty->tty_left_count = msg->CNT; //请求数据大小
	tty->tty_trans_count = 0;	//已经传输了多少
	msg->type = SUSPEND_PROC;	//挂起TASK_FS进程
	msg->CNT = tty->tty_left_count;	
	send_recv(SEND, tty->tty_caller, msg); //向TASK_FS发送消息
}
//tty_do_write作用是将字符write到dev_tty文件，最终显示在屏幕上
void tty_do_write(struct tty * tty, struct message *msg)
{
	char buf[TTY_OUT_BUF_LEN];
	char *p = (char*)va2la(msg->PID, msg->BUF);
	int i = msg->CNT;
	int j;
	while(i){
		int bytes = min(TTY_OUT_BUF_LEN, i);
		memcpy(va2la(TASK_TTY, buf), (void*)p, bytes);
		for(j=0;j<bytes;j++){
			out_char(tty->p_console, buf[j]);
		}
		i-=bytes;
		p+=bytes;
	}
	msg->type = SYSCALL_RET;
	send_recv(SEND, msg->source, msg);
}
/**
 * @function init_tty
 * @brief 初始化tty
 * @param p_tty tty指针
 *
 */
void init_tty(struct tty *p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;
	init_screen(p_tty);
}

/**
 * @function put_key
 * @brief 将扫描码放入tty的输入缓冲区
 * @param p_tty tty指针
 * @param key  编码后的按键
 *
 */
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
 *系统调用，内核级打印
 */
int sys_printk(int _unsed1, int _unsed2, char *s, struct process *p_proc)
{
	char * p;
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
		//如果是panic 或者内核级别的assert，那么执行这里
		_disable_int();
	#ifdef _SHOW_PANIC_
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
	#endif
		__asm__ __volatile__("hlt");
	}
	/*
	while((ch=*p++)!=0){
		if(ch==MAG_CH_PANIC ||ch==MAG_CH_ASSERT) continue;
		//out_char(tty_table[p_proc->tty_idx].p_console, ch);
		out_char(tty_table[0].p_console, ch); //默认使用tty0
	}
	*/
	if(*p==MAG_CH_PANIC || *p == MAG_CH_ASSERT) p++;
        tty_write(&tty_table[0],p,strlen(p));
	return 0;
}
