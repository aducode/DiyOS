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
 * @struct floppy_struct
 * @brief 支持软盘类型的参数
 *
 * 参考linux 0.11内核
 */
static struct floppy_struct {
	unsigned int size;	//大小（扇区数）
	unsigned int sect;	//每磁道扇区数
	unsigned int head;	//磁头数
	unsigned int track;	//磁道数
	unsigned int stretch;	//对磁道是否需要特殊处理
	unsigned char gap;	//数据区域长度(字节数)
	unsigned char rate;	//数据传输速率
	unsigned char specl;	//参数(高4位步进速率， 低4位磁头卸载时间)
} FD_144 = {2880, 18, 2, 80, 0, 0x1B, 0x00, 0xCF}; //我们只支持1.44MB软盘

/**
 * mount计数
 **/
static int floppy_mount_count;

static int recalibrate = 0; //1表示需要重新校正磁头位置

static int reset = 0; //1表示需要进行复位操作

//DATA_FIFO输出
static unsigned char reply_buffer[MAX_REPLIES];
#define ST0	reply_buffer[0]
#define ST1	reply_buffer[1]
#define ST2	reply_buffer[2]
#define ST3	reply_buffer[3]
/**
 * 处理中断
 */
static void floppy_handler(int irq_no);
static void reset_interrupt_handler(int irq_no);

/**
 * @function reset_floppy
 * @brief 重置软盘驱动并设置参数
 * @param dev 软驱号0-3
 */
static void reset_floppy(int dev);

/**
 * @function fdc_output_byte
 * @brief 向FDC（软盘控制器)写入byte
 * @param byte 要写入的字节
 */
static void fdc_output_byte(char byte);

/**
 * @function fdc_resul
 * @biref 获取FDC执行结果
 * @return result
 */
static int fdc_result();


/********************************************* PUBLIC ***************************************/

/**
 * @function init_floppy
 * @brief 初始化软盘，开启软盘中断
 */
void init_floppy()
{
	//初始化floppy_mount_count
	floppy_mount_count = 0;
	//设置FDC基本参数
	//BIOS已经设置过这些参数了，不需要再设置
	//fdc_output_byte(CMD_CONFIGURE);
	//fdc_output_byte(0);
	//fdc_output_byte((1<<6)|(1<<5)|(0<<4)|(8));
	//fdc_output_byte(0);
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
		reset_floppy(device);
		//说明是第一次mount
		//int i;
		//setp 1 enable interrupt
		//_disable_irq(FLOPPY_IRQ);
		//irq_handler_table[FLOPPY_IRQ] = floppy_handler;
		//_enable_irq(FLOPPY_IRQ);	
		//step 2 reset
		//_out_byte(DIGITAL_OUTPUT_REGISTER, CMD_FD_RECALIBRATE);//开始重置软盘控制器命令
		//_out_byte(DIGITAL_OUTPUT_REGISTER, device); //指定驱动器号 
		//for(i=0;i<100;i++){
		//	__asm__("nop");//延时保证重启完成
		//}
		//_out_byte(DIGITAL_OUTPUT_REGISTER, 0x0c);			//选择DMA模式，选择软驱A
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


void fdc_output_byte(char byte)
{
	int counter; 		//计数器
	unsigned char status; //主状态寄存器的值
	
	if(reset){
		//如果需要重置，那么直接返回
		return;
	}
	//循环不断轮询主状态寄存器的值，如果status ready并且方向是由CPU->FDC,说明可以写入
	for(counter = 0;counter < 10000; counter++){
		status = _in_byte(MAIN_STATUS) & (STATUS_READY | STATUS_DIR);
		if(status == STATUS_READY) {
			//说明状态就绪，并且方向是由CPU->FDC
			_out_byte(DATA_FIFO, byte);
			return;
		}
	}
	reset = 1; //出错，需要重置
	printk("Unable send byte to FDC\n");
}

int fdc_result()
{
	int i = 0, counter, status;
	if(reset) {
		return -1;
		//如果需要重置，那么立刻返回
	}
	for(counter = 0; counter < 10000; counter++){
		status = _in_byte(MAIN_STATUS) & (STATUS_READY | STATUS_DIR | STATUS_BUSY);
		if(status == STATUS_READY){
			//表示DATA_FIFO没有更多数据，此时返回已经读取的字节数
			return i;
		}
		if(status == (STATUS_READY | STATUS_DIR | STATUS_BUSY)){
			//表示有数据可读
			if(i >= MAX_REPLIES) {
				break;
			}
			reply_buffer[i++] = _in_byte(DATA_FIFO);		
		}
	}
	//循环10000次，则超时
	reset = 1;
	printk("Get status timeout\n");
	return -1;
}

void reset_interrupt_handler(int irq_no)
{
	//检查中断状态
	fdc_output_byte(CMD_SENSEI_INTERRUPT);
	fdc_result(); 
	//不需要结果
	//重新设置参数
	fdc_output_byte(CMD_SPECIFY);
	fdc_output_byte(FD_144.specl);
	fdc_output_byte(2<<1|0); //磁头加载时间2*4ms， DMA
/*	
	_disable_irq(FLOPPY_IRQ);
	irq_handler_table[FLOPPY_IRQ] = floppy_handler;
	_enable_irq(FLOPPY_IRQ);
*/
}

void reset_floppy(int dev)
{
	reset = 0;
	recalibrate = 1;
	int i;
	//关中断
	_disable_irq(FLOPPY_IRQ);
	irq_handler_table[FLOPPY_IRQ] = reset_interrupt_handler;//floppy_handler;
	//重启
	//在对软盘进行操作之前，必须先“选中”对应磁盘，并开启马达
	_out_byte(DIGITAL_OUTPUT_REGISTER, BUILD_DOR(dev, OFF, 1, OFF));
	for(i=0;i<100;i++){
		__asm__("nop");
	}
	_out_byte(DIGITAL_OUTPUT_REGISTER, BUILD_DOR(dev, OFF, 1, ON));	//再启动
	//开中断
	_enable_irq(FLOPPY_IRQ);
	//VERSION CMD
	fdc_output_byte(CMD_VERSION);
	int bytes = fdc_result();
	//保证CMD VERSION返回1byte，并且值为0x90
	//也就是运行在软盘控制芯片82077AA
	assert(bytes==1 && ST0 == 0x90);
}
