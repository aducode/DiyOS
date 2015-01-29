#include "type.h"
#include "klib.h"
#include "global.h"

/**
 * 处理键盘硬件中断
 */
static void keyboard_handler(int irq_no);


void keyboard_handler(int irq_no)
{
	static int color = 0x00;
	_in_byte(0x60);	//清空8042缓冲才能下一次中断
	_disp_str("Press any key :)",10,0,color++);
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
