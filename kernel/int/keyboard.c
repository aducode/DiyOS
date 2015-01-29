#include "type.h"
#include "klib.h"
#include "global.h"
#include "string.h"
#include "keyboard.h"
/**
 * 处理键盘硬件中断
 */
static void keyboard_handler(int irq_no);
//
static struct keyboard_buffer_t keyboard_buffer;

static void init_kb();
static void put_code_into_buffer();
static u8 get_code_from_buffer();
void keyboard_handler(int irq_no)
{
	//u8 scan_code = _in_byte(IO_8042_PORT);	//清空8042缓冲才能下一次中断
	put_code_into_buffer();
}

/**
 * 初始化键盘
 */
void init_keyboard()
{
	//初始化全局键盘缓冲区，在global.c中
	init_kb();
	//开启键盘硬件中断响应
	_disable_irq(KEYBOARD_IRQ);
	irq_handler_table[KEYBOARD_IRQ] = keyboard_handler;
	_enable_irq(KEYBOARD_IRQ);	
}


//操作键盘缓冲区

/**
 * 初始化
 */
void init_kb()
{
	keyboard_buffer.count = 0;
	keyboard_buffer.p_head = keyboard_buffer.p_tail = keyboard_buffer.buffer;
}

/**
 * 将键盘扫描码放入缓冲
 */
void put_code_into_buffer()
{	u8 scan_code = _in_byte(IO_8042_PORT);
	if(keyboard_buffer.count < KEYBOARD_BUFFER_SIZE){
		*(keyboard_buffer.p_head) = scan_code;
		keyboard_buffer.p_head++;
		if(keyboard_buffer.p_head == keyboard_buffer.buffer+KEYBOARD_BUFFER_SIZE){
			keyboard_buffer.p_head = keyboard_buffer.buffer;
		}
		keyboard_buffer.count++;		
	}
}

/**
 *从缓冲中获取键盘扫描码
 */
u8 get_code_from_buffer()
{
	u8 scan_code;
	while(keyboard_buffer.count<=0){
		//block wait for code
	}
	//这里需要关中断，避免重入中断产生数据不一致
	_disable_int();
	scan_code = *(keyboard_buffer.p_tail);
	keyboard_buffer.p_tail ++;
	if(keyboard_buffer.p_tail == keyboard_buffer.buffer+KEYBOARD_BUFFER_SIZE){
		keyboard_buffer.p_tail = keyboard_buffer.buffer;
	}
	keyboard_buffer.count --;
	_enable_int();	
	return scan_code;
}



//////////////////////////////////
/**
 * 供task/tty.c中使用，用于显示keyboard input字符串
 */
void keyboard_read(){
        static int color = 0x00;
        static char msg[256];

	u8 scan_code;
	char output[2];
	int make;
	_memset(output,0,2);
	if(keyboard_buffer.count>0){
		_disable_int();
	
	        scan_code = get_code_from_buffer();
		
		_enable_int();
		//
		if(scan_code == 0xE1) {
			//ignore
		} else if (scan_code == 0xE0) {
			//ignore
		} else {
			//make = (scan_code & FLAG_BREAK ?FALSE:TRUE);
		}	
        	_disp_str("Press a key :)",10,0,color++);
	        itoa(scan_code, msg,16);
	        _disp_str(msg,11,0,COLOR_WHITE);
	        if(color>0xFF)color=0x00;
	}
}

