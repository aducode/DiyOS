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
	//static char msg[20];
	//_disp_str("IN RING0 CLOCKINT...  ticks now is:",22,0,COLOR_RED);
	if((ticks += TICK_SIZE)>MAX_TICKS) ticks = 0;
	if(p_proc_ready->ticks) p_proc_ready->ticks--;
//	_clean(0,0,25,80);
	//itoa((int)ticks,msg, 10);
	if(key_pressed){
		inform_int(TASK_TTY);
	}
	schedule();
}
void init_clock()
{
	ticks = 0;
	_out_byte(TIMER_MODE, RATE_GENERATOR);
	_out_byte(TIMER0,(u8)(LATCH & 0xFF));
	_out_byte(TIMER0,(u8)(((LATCH)>>8)) & 0xFF);

	_disable_irq(CLOCK_IRQ);
	irq_handler_table[CLOCK_IRQ] = clock_handler;
	_enable_irq(CLOCK_IRQ);
}
