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
 * @return
 */
static void rdwt_floppy(int type, int dev, int head, int cylinder, int sector, int sector_count);
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
	rdwt_floppy(msg->type, dev, H(sector_nr), C(sector_nr), S(sector_nr),bytes/512+(bytes%512==0)?0:1);
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
		fdc_output_byte(FD_144.specl);
		fdc_output_byte(2<<1|0); //磁头加载时间2*4ms， DMA
	}
}

void reset_floppy(int dev)
{
	fdc_status.reset = 0;
	fdc_status.recalibrate = 1;
	fdc_status.curr_dev = dev;
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

void recalibrate_interrupt_handler(int riq_no)
{
	fdc_output_byte(CMD_SENSEI_INTERRUPT);
	if(fdc_result()!=2 || ST0 & (0x20 | fdc_status.curr_dev) != (0x20 | fdc_status.curr_dev)) {
		printk("0x%x\n", ST0);
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
	_disable_irq(FLOPPY_IRQ);	
	irq_handler_table[FLOPPY_IRQ] = recalibrate_interrupt_handler;
	_enable_irq(FLOPPY_IRQ);
	//CMD RECALIBRATE
	fdc_output_byte(CMD_RECALIBRATE);
	fdc_output_byte(dev);
	if(fdc_status.reset){
		//fdc_output出错了，需要重置
		reset_floppy(dev);
	} else{
		sleep(3000);
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
	printk("seek..%d,%d\n", head, cylinder);
	if(fdc_status.reset){
		return;
	}
	fdc_status.seek = 0;
	if(fdc_status.curr_head == head && fdc_status.curr_cylinder == cylinder){
		return;
	}
	_disable_irq(FLOPPY_IRQ);
	irq_handler_table[FLOPPY_IRQ] = seek_interrupt_handler;
	_enable_irq(FLOPPY_IRQ);
	
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
		sleep(3000);
	}
}

void rdwt_interrupt_handler(int irq_no)
{
	printk("rdwt interrupt!\n");
}

void rdwt_floppy(int type,  int dev, int head, int cylinder, int sector, int sector_count)
{
	if(fdc_status.reset == 1 || fdc_status.recalibrate == 1|| fdc_status.seek == 1){
		return;
	}
	if(type!=DEV_READ && type!=DEV_WRITE){
		return;
	}

	_disable_irq(FLOPPY_IRQ);
	irq_handler_table[FLOPPY_IRQ] = rdwt_interrupt_handler;
	_enable_irq(FLOPPY_IRQ);
	
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
		sleep(10000);
	}	
		
}
