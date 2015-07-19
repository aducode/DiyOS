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
	//empty
}

/**
 * @function floppy_open
 * @brief 打开设备
 * @param device 设备号
 * @return
 */
void floppy_open(int device)
{
	int i;
	//setp 1 enable interrupt
	_disable_irq(FLOPPY_IRQ);
	irq_handler_table[FLOPPY_IRQ] = floppy_handler;
	_enable_irq(FLOPPY_IRQ);	
	//step 2 reset
	_out_byte(FD_DOR, 0x08);		//重启
	for(i=0;i<100;i++){
		__asm__("nop");//延时保证重启完成
	}
	_out_byte(FD_DOR, 0x0c);			//选择DMA模式，选择软驱A
	
}

/**
 * @function floppy_close
 * @brief 关闭设备
 * @param device 设备号
 * @return
 */
void floppy_close(int device)
{
	//disable irq
	_disable_irq(FLOPPY_IRQ);
	//rest irq handler
	irq_handler_table[FLOPPY_IRQ] = default_irq_handler;	
}

/**
 * @function floppy_rdwt
 * @brief 设备读写数据
 * @param msg 消息
 * @return
 */
void floppy_rdwt(struct message *msg)
{
	
}

/**
 * @function floppy_ioctl
 * @brief 控制设备
 * @param msg 消息
 */
void floppy_ioctl(struct message *msg)
{
	
}

/********************************************* PRIVATE ************************************/

void floppy_handler(int irq_no)
{
	printk("[*]floppy irq:%d\n", irq_no);
}
