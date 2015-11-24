/*****************************************************************/
/*                   软盘中断&软盘寄存器操作                     */
/*****************************************************************/
// 这里的操作与最开始boot不同，boot使用的是BIOS软盘中断，这里需要直接操作软盘寄存器地址，进行控制
#include "type.h"
#include "string.h"
#include "global.h"
#include "proc.h"
#include "syscall.h"
#include "klib.h"
#include "assert.h"
#include "floppy.h"

/**
 * mount计数
 **/
static int floppy_mount_count;
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
	//初始化floppy_mount_count
	floppy_mount_count = 0;
}

/**
 * @function floppy_open
 * @brief 打开设备
 * @param device 设备号
 * @return
 */
void floppy_open(int device)
{
	//之前考虑的mount /dev/floppy /test 时，是应用层open函数调用至此
	//参考linux，mount应该是一个系统调用与open同级别，所以打开中断与关闭中断应该在mount / unmount 中 而不应该在open close中
	//应该是mount 的时候调用至此
	if(floppy_mount_count == 0){
		//说明是第一次mount
		int i;
		//setp 1 enable interrupt
		_disable_irq(FLOPPY_IRQ);
		irq_handler_table[FLOPPY_IRQ] = floppy_handler;
		_enable_irq(FLOPPY_IRQ);	
		//step 2 reset
		_out_byte(DIGITAL_OUTPUT_REGISTER, 0x08);		//重启
		for(i=0;i<100;i++){
			__asm__("nop");//延时保证重启完成
		}
		_out_byte(DIGITAL_OUTPUT_REGISTER, 0x0c);			//选择DMA模式，选择软驱A
	} else {
		//说明已经mount过了
	}
	//mount操作之后mount计数自增
	floppy_mount_count += 1;
}

/**
 * @function floppy_close
 * @brief 关闭设备
 * @param device 设备号
 * @return
 */
void floppy_close(int device)
{
	// unmount /dev/floppy 的fd时调用
	//由于一个分区（比如这里的floppy）可以挂在到多个目录
	//所以一个unmount之后不能确定还有没有其他的mount目录，所以不能一概而论的disable中断
	//设置一个static变量，来表示mount该分区的数量
	//floppy_mount_count 为0表示可以disable中断
	floppy_mount_count -= 1;
	if(floppy_mount_count==0){
		//TODO 关闭软盘设备
		
		//disable irq
		_disable_irq(FLOPPY_IRQ);
		//rest irq handler
		irq_handler_table[FLOPPY_IRQ] = default_irq_handler;
	} else {
		//说用还有其他位置mount floppy
	}
}

/**
 * @function floppy_rdwt
 * @brief 设备读写数据
 * @param msg 消息
 * @return
 */
void floppy_rdwt(struct message *msg)
{
	printk("floppy_rdwt\n");	
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
