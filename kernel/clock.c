#include "type.h"
#include "clock.h"
#include "klib.h"
#include "global.h"
#include "interrupt.h"
#include "proc.h"
static void clock_handler(int irq_no);
/**
 * 时钟中断处理函数
 */
void clock_handler(int irq_no)
{	
	static char msg[20];
	_disp_str("IN RING0 CLOCKINT...  ticks now is:",4,0,COLOR_RED);
	ticks+=1;
	if(ticks>MAX_TICKS)ticks=0;
//	_clean(0,0,25,80);
	itoa((int)ticks,msg, 10);
	_disp_str(msg,5,0,COLOR_RED);
	schedule();
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
