#ifndef _DIYOS_KEYBOARD_H
#define _DIYOS_KEYBOARD_H
/** 8042寄存器 **/
#define IO_8042_PORT	0x60
#define CTL_8042_PORT	0x64
extern void init_keyboard();
//键盘环形缓冲
#define KEYBOARD_BUFFER_SIZE	32
struct keyboard_buffer_t{
	char * p_head;	//头
	char * p_tail;  //尾
	int count;	//计数器
	char buffer[KEYBOARD_BUFFER_SIZE];
};
extern void keyboard_read();
#endif
