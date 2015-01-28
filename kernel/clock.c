#include "type.h"
#include "clock.h"
#include "klib.h"
#include "global.h"
#include "interrupt.h"

static void clock_handler(int irq_no);
/**
 * 时钟中断处理函数
 */
void clock_handler(int irq_no)
{
	ticks++;
	if(ticks>MAX_TICKS)ticks=0;
	_clean(0,0,25,80);
	_disp_str("IN CLOCK HANDLER yooooo!!!!",(ticks&24)+1,0,COLOR_WHITE);	
}
void init_clock()
{
	ticks = 0;
	_out_byte(TIMER_MODE, RATE_GENERATOR);
	_out_byte(TIMER0,(u8)(TIMER_FREQ/HZ));
	_out_byte(TIMER0,(u8)(TIMER_FREQ/HZ>>8));

	_disable_irq(CLOCK_IRQ);
	irq_handler_table[CLOCK_IRQ] = clock_handler;
	_enable_irq(CLOCK_IRQ);
}
