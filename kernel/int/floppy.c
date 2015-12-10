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
#include "concurrent.h"
#include "system.h"

extern char temp_floppy_area[1024];

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
	unsigned char spec1;	//参数(高4位步进速率， 低4位磁头卸载时间)
	unsigned char spec2;	//6表示DMA
} FD_144 = {2880, 18, 2, 80, 0, 0x1B, 0x00, 0xCF, 6}; //我们只支持1.44MB软盘

/**
 * mount计数
 **/
static int floppy_mount_count;

static struct fdc_status_struct {
	int reset;		//当前需要reset，1表示需要进行复位操作
	int recalibrate;	//需要recalibrate, 1表示需要重新校正磁头位置
	int seek;		//当前需要seek，1表示需要seek
	int curr_dev;	//当前的软驱号
	//软盘CHS
	int curr_head;	//当前磁头
	int curr_cylinder;	//当前柱面
	int curr_sector;	//当前扇区
} fdc_status = {0, 0, 0, 0, 0, 0, 0};  //fdc的状态


//DATA_FIFO输出
static unsigned char reply_buffer[MAX_REPLIES];
//The correct value of st0 after a reset should be 0xC0 | drive number (drive number = 0 to 3). After a Recalibrate/Seek it should be 0x20 | drive number.
//st0
//The top 2 bits (value = 0xC0) are set after a reset procedure, with polling on. Either bit being set at any other time is an error indication. Bit 5 (value = 0x20) is set after every Recalibrate, Seek, or an implied seek. The other bits are not useful.
#define ST0	reply_buffer[0]
//st1
//The st1 register provides more detail about errors during read/write operations.
//Bit 7 (value = 0x80) is set if the floppy had too few sectors on it to complete a read/write. This is often caused by not subtracting 1 when setting the DMA byte count. Bit 4 (value = 0x10) is set if your driver is too slow to get bytes in or out of the FIFO port in time. Bit 1 (value = 2) is set if the media is write protected. The rest of the bits are for various types of data errors; indicating bad media, or a bad drive.
#define ST1	reply_buffer[1]
//st2
//The st2 register provides more (useless) detail about errors during read/write operations.
//The bits all indicate various types of data errors for either bad media, or a bad drive.
#define ST2	reply_buffer[2]

//根据物理上的磁头/磁道/扇区获得相对扇区号
#define SECTOR(head, cylinder, sector) ((head) * 18 + 2 * (cylinder) * 18 + (sector) -1)
//相对扇区号对每条磁道扇区数(18)取模加1就得到扇区
#define S(sector) ((sector) % 18 + 1)
//
#define H(sector) (((sector) / 18) % 2 == 1 ? 1:0)
//
#define C(sector) ((sector)/18 / 2)
/**
 * 处理中断
 */
static void floppy_handler(int irq_no);
static void reset_interrupt_handler(int irq_no);
static void recalibrate_interrupt_handler(int irq_no);
static void seek_interrupt_handler(int irq_no);
static void rdwt_interrupt_handler(int irq_no);

/**
 * @function setup_DMA
 * @brief 设置DMA
 *        参考linux kernel 0.11
 * @param type READ/WRITE
 * @param buf
 */
static void setup_DMA(int type, void * buf);

/**
 * @function switch_floppy_motor
 * @brief 开/关 软驱马达
 * @param dev
 * @param sw ON/OFF
 * @return
 */
static void switch_floppy_motor(int dev, int sw);

/**
 * @function reset_floppy
 * @brief 重置软盘驱动并设置参数
 * @param dev 软驱号0-3
 */
static void reset_floppy(int dev);

/**
 * @function recalibrate_floppy
 * @brief 磁头归零
 * @param dev
 * @return void
 */
static void recalibrate_floppy(int dev);

/**
 * @function seek_floppy
 * @brief 寻道
 * @param dev
 * @param head 磁头
 * @param cylinder
 */
static void seek_floppy(int dev, int head, int cylinder);

/**
 * @function rdwt_floppy
 * @brief 读写
 * @param type
 * @param dev
 * @param head
 * @param cylinder
 * @param sector
 * @param sector_count读/写的扇区数
 * @param buf
 * @return
 */
static void rdwt_floppy(int type, int dev, int head, int cylinder, int sector, int sector_count, void * buf);
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
void do_init_floppy()
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
void do_floppy_open(int device)
{
	//之前考虑的mount /dev/floppy /test 时，是应用层open函数调用至此
	//参考linux，mount应该是一个系统调用与open同级别，所以打开中断与关闭中断应该在mount / unmount 中 而不应该在open close中
	//应该是mount 的时候调用至此
	if(floppy_mount_count == 0){
		reset_floppy(device);
		recalibrate_floppy(device);
		//不需要重置/重定位
		assert(fdc_status.reset == 0 && fdc_status.recalibrate==0);
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
void do_floppy_close(int device)
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
void do_floppy_rdwt(struct message *msg)
{
	int dev = msg->DEVICE;
	assert(dev>=0 && dev <=3);//0～3
	u64 pos = msg->POSITION;	//起始位置
	//pos必须是扇区开始处
	assert((pos & 0x1FF) == 0);
	int bytes = msg->CNT;		//字节数
	//每个扇区512字节
	int sector_nr = pos / 512;	//这里得到扇区号
	//read
	seek_floppy(dev, H(sector_nr), C(sector_nr));
	rdwt_floppy(msg->type, dev, H(sector_nr), C(sector_nr), S(sector_nr),bytes/512+(bytes%512==0)?0:1, (void*)va2la(msg->PID,msg->BUF));
}

/**
 * @function floppy_ioctl
 * @brief 控制设备
 * @param msg 消息
 */
void do_floppy_ioctl(struct message *msg)
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
	
	if(fdc_status.reset){
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
	fdc_status.reset = 1; //出错，需要重置
	printk("Unable send byte to FDC\n");
}

int fdc_result()
{
	int i = 0, counter, status;
	if(fdc_status.reset) {
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
	fdc_status.reset = 1;
	printk("Get status timeout\n");
	return -1;
}

void reset_interrupt_handler(int irq_no)
{
	//检查中断状态
	fdc_output_byte(CMD_SENSEI_INTERRUPT);
	if(fdc_result()!=2 || ST0 != 0xC0){
		fdc_status.reset = 1;//说明reset失败
	} else {
		//不需要结果
		//重新设置参数
		fdc_output_byte(CMD_SPECIFY);
		fdc_output_byte(FD_144.spec1);
		fdc_output_byte(FD_144.spec2); //磁头加载时间6ms， DMA
	}
}

void reset_floppy(int dev)
{
	fdc_status.reset = 0;
	fdc_status.recalibrate = 1;
	fdc_status.curr_dev = dev;
	int i;
	//关中断
	//_disable_irq(FLOPPY_IRQ);
	cli();
	irq_handler_table[FLOPPY_IRQ] = reset_interrupt_handler;//floppy_handler;
	//重启
	//在对软盘进行操作之前，必须先“选中”对应磁盘，并开启马达
	_out_byte(DIGITAL_OUTPUT_REGISTER, BUILD_DOR(dev, OFF, 1, OFF));
	for(i=0;i<100;i++){
		nop();
	}
	//在读写之前需要向DOR port输出BUILD_DOR(dev, ON, 1, ON);
	//否则会出现motor not on的错误
	_out_byte(DIGITAL_OUTPUT_REGISTER, BUILD_DOR(dev, OFF, 1, ON));	//再启动
	//开中断
	//_enable_irq(FLOPPY_IRQ);
	sti();
	//VERSION CMD
	fdc_output_byte(CMD_VERSION);
	int bytes = fdc_result();
	//保证CMD VERSION返回1byte，并且值为0x90
	//也就是运行在软盘控制芯片82077AA
	assert(bytes==1 && ST0 == 0x90);
}

void recalibrate_interrupt_handler(int riq_no)
{
	fdc_output_byte(CMD_SENSEI_INTERRUPT);
	if(fdc_result()!=2 || ST0 & (0x20 | fdc_status.curr_dev) != (0x20 | fdc_status.curr_dev)) {
		// Bit 5 (value = 0x20) is set after every Recalibrate, Seek, or an implied seek
		fdc_status.reset = 1;
	} else {
		fdc_status.recalibrate = 0;
	}
}

void recalibrate_floppy(int dev)
{
	if(fdc_status.reset){
		return;
	}
	fdc_status.curr_dev = dev;
	fdc_status.recalibrate = 0;
	//_disable_irq(FLOPPY_IRQ);
	//cli();
	irq_handler_table[FLOPPY_IRQ] = recalibrate_interrupt_handler;
	//_enable_irq(FLOPPY_IRQ);
	//sti();
	//CMD RECALIBRATE
	fdc_output_byte(CMD_RECALIBRATE);
	fdc_output_byte(dev);
	if(fdc_status.reset){
		//fdc_output出错了，需要重置
		reset_floppy(dev);
	} else{
		sleep(300);
	}
}

void seek_interrupt_handler(int irq_no)
{
	fdc_output_byte(CMD_SENSEI_INTERRUPT);
	if(fdc_result()!=2 || ST0 & (0x20 | fdc_status.curr_dev) != (0x20 | fdc_status.curr_dev)){
		fdc_status.reset = 1;
	} else {
		fdc_status.seek = 0;
	}
}

void seek_floppy(int dev, int head, int cylinder)
{
	if(fdc_status.reset){
		return;
	}
	fdc_status.seek = 0;
	if(fdc_status.curr_head == head && fdc_status.curr_cylinder == cylinder){
		return;
	}
	//_disable_irq(FLOPPY_IRQ);
	//cli();
	irq_handler_table[FLOPPY_IRQ] = seek_interrupt_handler;
	//_enable_irq(FLOPPY_IRQ);
	//sti();
	
	//CMD SEEK
	fdc_output_byte(CMD_SEEK);
	fdc_output_byte(fdc_status.curr_head << 2| fdc_status.curr_dev);
	fdc_output_byte(fdc_status.curr_cylinder);
	if(fdc_status.reset){
		reset_floppy(dev);	
	} else {
		fdc_status.curr_dev = dev;
		fdc_status.curr_head = head;
		fdc_status.curr_cylinder = cylinder;
		sleep(300);
	}
}

void rdwt_interrupt_handler(int irq_no)
{
	printk("rdwt interrupt!\n");
}

void rdwt_floppy(int type,  int dev, int head, int cylinder, int sector, int sector_count, void * buf)
{
	if(fdc_status.reset == 1){
		reset_floppy(dev);
		return;
	}
	if(fdc_status.recalibrate == 1){
		recalibrate_floppy(dev);
		return;
	}
	if(type!=DEV_READ && type!=DEV_WRITE){
		return;
	}
	setup_DMA(type, buf);
	switch_floppy_motor(dev, ON);
	//_disable_irq(FLOPPY_IRQ);
	//cli();
	irq_handler_table[FLOPPY_IRQ] = rdwt_interrupt_handler;
	//_enable_irq(FLOPPY_IRQ);
	//sti();
	
	fdc_output_byte((type==DEV_READ)?(CMD_READ):(CMD_WRITE));
	fdc_output_byte(head<<2|dev);
	fdc_output_byte(cylinder);
	fdc_output_byte(head);
	fdc_output_byte(sector);
	fdc_output_byte(2);	//all floppy drives use 512bytes per sector
	fdc_output_byte(sector_count);
	fdc_output_byte(FD_144.gap);
	fdc_output_byte(0xFF);
	if(fdc_status.reset){
		//说明有错误
		reset_floppy(dev);
	} else {
		fdc_status.curr_dev =dev;
		fdc_status.curr_head = head;
		fdc_status.curr_cylinder = cylinder;
		fdc_status.curr_sector = sector;	
		sleep(1000);
	}
	//读写的函数都能正确的运行，同时上面也设置了读写中断处理函数
	//可是rdwt_interrupt_handler函数不会被调用
	//还不知道是因为什么，由于软盘设备比较古老，用起来也没有像硬盘设备那么便捷，资料也不是很多，还需要好好研究
	//
	//能够挂载软盘，是便于进行后面拆分操作系统内核和应用做准备，毕竟floppy镜像可以在linux中挂载，向里面拷贝一些程序也比较简单，同时接入操作系统的抽象文件系统(Abstract File System),也能体现分层次的好处啦～
	//方案2也有，那就是在写个程序，能向我们的硬盘里面直接写入数据，但是这中做法总觉得不太完美，毕竟要做的是个操作系统，可不想让它以来太多其他的程序。
}

void switch_floppy_motor(int dev, int sw)
{
	int i;
	if(fdc_status.reset){
		return;
	}
	//_disable_irq(FLOPPY_IRQ);
	cli();
	_out_byte(DIGITAL_OUTPUT_REGISTER, BUILD_DOR(dev, ON, 1, sw));
        for(i=0;i<100;i++){
		nop();
        }
	//_enable_irq(FLOPPY_IRQ);
	sti();
}


/**
 *  Copy from linux kernel 0.11
 */
void setup_DMA(int type, void * buf)
{
	long addr = (long)buf;
	cli();
	if(addr > 0x100000){
		//DMA只能在0x100000以内寻址
		addr = (long)temp_floppy_area;
		if(type == DEV_WRITE){
			memcpy(temp_floppy_area, buf, 1024);
		}	
	}
	//mask DMA 2
	_out_byte(10, 4|2);
	/* output command byte. I don't know why, but everyone (minix, */
	/* sanches & canton) output this twice, first to 12 then to 11 */
        __asm__("outb %%al,$12\n\tjmp 1f\n1:\tjmp 1f\n1:\t"
        "outb %%al,$11\n\tjmp 1f\n1:\tjmp 1f\n1:"::
        "a" ((char) ((type == DEV_READ)?DMA_READ:DMA_WRITE)));
	//0~7bits of addr
	_out_byte(4, addr);
	//8~15 bits of addr
	addr >>= 8;
	_out_byte(4, addr);
	//16~19 bits of addr
	addr >>= 8;
	_out_byte(0x81, addr);
	//low 8 bits of count - 1 (1024 - 1 = 0x3FF) 
	_out_byte(5, 0xFF);
	//high 8 bits of count - 1
	_out_byte(5, 0x03);
	//active DMA 2
	_out_byte(10, 0|2);
	sti();
}
