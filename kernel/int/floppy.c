/*****************************************************************/
/*                      软盘中断                                 */
/*****************************************************************/
#include "type.h"
#include "string.h"
#include "global.h"
#include "proc.h"
#include "syscall.h"
#include "klib.h"
#include "assert.h"
#include "floppy.h"

/**
 * 处理中断
 */
static void floppy_handler(int irq_no);

/********************************************* PUBLIC ***************************************/

/**
 * @function init_floppy
 * @brief 初始化软盘，开启软盘中断
 */
void init_floppy()
{
	_disable_irq(FLOPPY_IRQ);
	irq_handler_table[FLOPPY_IRQ] = floppy_handler;
	_enable_irq(FLOPPY_IRQ);	
}

/********************************************* PRIVATE ************************************/

void floppy_handler(int irq_no)
{
	printk("floppy irq:%d\n", irq_no);
}
