#include "type.h"
#include "klib.h"
#include "global.h"
#include "string.h"
#include "keyboard.h"
/**
 * 处理键盘硬件中断
 */
static void keyboard_handler(int irq_no);


void keyboard_handler(int irq_no)
{
	static int color = 0x00;
	static char msg[256];
	u8 scan_code = _in_byte(IO_8042_PORT);	//清空8042缓冲才能下一次中断
	_disp_str("Press a key :)",10,0,color++);
	itoa(scan_code, msg,16);
	_disp_str(msg,11,0,COLOR_WHITE);
	if(color>0xFF)color=0x00;	
}

/**
 * 初始化键盘
 */
void init_keyboard()
{
	_disable_irq(KEYBOARD_IRQ);
	irq_handler_table[KEYBOARD_IRQ] = keyboard_handler;
	_enable_irq(KEYBOARD_IRQ);	
}
