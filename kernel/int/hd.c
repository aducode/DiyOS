/************************************************************/
/*                       硬盘中断                           */
/************************************************************/
#include "type.h"
#include "global.h"
#include "klib.h"
#include "assert.h"
#include "string.h"
static void hd_handler(int irq_no);
void init_hd()
{
	char msg[20];
	itoa(hdinfo.hd_num, msg, 10);
	_disp_str(msg, 23,0, COLOR_YELLOW);
	assert(hdinfo.hd_num>0);
	_disable_irq(AT_WINI_IRQ);
	irq_handler_table[AT_WINI_IRQ] = hd_handler;
	_enable_irq(CASCADE_IRQ);
	_enable_irq(AT_WINI_IRQ);
}
void hd_handler(int irq_no)
{
	_disp_str("hd",23,0,COLOR_YELLOW);
}

