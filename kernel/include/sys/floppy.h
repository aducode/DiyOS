#include "type.h"
#ifndef _DIYOS_FLOPPY_H
#define _DIYOS_FLOPPY_H

//软盘设备号
#define MINOR_flda		0x00			//A盘
#define FLOPPYA_DEV		MAKE_DEV(DEV_FLOPPY, MINOR_flda)	//只有一个软盘

/**
 * @function init_floppy
 * @brief 初始化floppy 中断
 * @return
 */
extern void do_init_floppy();

/**
 * @function floppy_open
 * @brief 打开设备
 * @param device 设备号
 * @return
 */
extern void do_floppy_open(int device);

/**
 * @function floppy_close
 * @brief 关闭设备
 * @param device 设备号
 * @return
 */
extern void do_floppy_close(int device);

/**
 * @function floppy_rdwt
 * @brief 设备读写数据
 * @param msg 消息
 * @return
 */
extern void do_floppy_rdwt(struct message *msg);

/**
 * @function floppy_ioctl
 * @brief 控制设备
 * @param msg 消息
 */
extern void do_floppy_ioctl(struct message *msg);

/**
 * 参考资料:
 * @see [Floppy Disk Control](http://wiki.osdev.org/Floppy_Disk_Controller)
 */

//定义port 地址
#define 	STATUS_REGISTER_A		0x3f0		//read-only
#define		STATUS_REGISTER_B		0x3f1		//read-only
#define		DIGITAL_OUTPUT_REGISTER		0x3f2		//数字输出寄存器
#define		TAPE_DRIVE_REGISTER		0x3f3		//As wiki say, it useless	
#define		MAIN_STATUS			0x3f4		//主状态寄存器端口read-only
#define		DATARATE_SELECT_REGISTER	0x3f4		//write-only
#define		DATA_FIFO			0x3f5		//数据端口

#define		DIGITAL_INPUT_REGISTER		0x3f7		//数据传输率控制寄存器 read-only
#define		CONFIGUATION_CONTROL_REGISTER	0x3f7		//write-only 数据传输率控制寄存器

//DIGITAL_OUTPUT_REGISTER 数字输出寄存器
//DOR是一个8为寄存器，他控制驱动器马达的开启、驱动器选择、启动/复位FDC以及允许/禁止DMA请求
//位			Name		Description
//7			MOT_EN3		Driver D motor：1-start；0-stop
//6			MOT_EN2		Driver C motor：1-start；0-stop
//5			MOT_EN1		Driver B motor：1-start；0-stop
//4			MOT_EN0		Driver A motor：1-start；0-stop
//3			DMA_INT		DMA interrupt； 1 enable; 0-disable
//2			RESET		FDC Reset
//1			DRV_SEL1	Select driver
//0			DRV_SEL0	Select driver
//
//FDC Status：FDC状态寄存器
//FDC status用于反映软盘驱动器FDC的基本状态。通常，在CPU想FDC发送命令或从FDC获取结果前，都要读取FDC的状态为，以判断当前的FDC data寄存器是否就需，以及确定数据传输方向。
//
//位			Name			Description
//7			RQM			Data ready: FDD ready
//6			DIO			Direction: 1 - FDD to CPU; 0 – CPU to FDD
//5			NDM			DMA set: 1-not DMA; 0-DMA
//4			CB			Controller busy
//3			DDB			Driver D busy
//2			DCB			Driver C busy
//1			DBB			Driver B busy
//0			DAB			Driver A busy
//
//DOR
#define			ON			1
#define			OFF			0
/**
 * @define BUILD_DOR
 * @brief 构建写入DOR(Data output register)的byte
 * 
 * @param dev 软盘驱动号
 * @param fdsw 软驱马达是否启动 ON  OFF
 * @param dma_int 是否允许DMA中断，0禁止 1允许
 * @param reset FDC 0 重置
 */ 
#define BUILD_DOR(dev, fdsw, dma_int, reset) 	((dev) & 3) | (((reset) & 1) <<2 ) | (((dma_int) & 1) << 3) | ((fdsw)==OFF?0:(1<<(((dev) & 3)+4)))

//MAIN_STATUS状态定义
#define		STATUS_BUSYMASK			0x0F		//driver busy
#define		STATUS_BUSY			0x10		//FDC 忙
#define		STATUS_DMA			0x20		//0-DMA model
#define		STATUS_DIR			0x40		//传输方向0 CPU 1 相反
#define		STATUS_READY			0x80		//数据寄存器就位

//DEFINE FDC CMD
//FDC支持的CMD
#define		CMD_READ_TRACK			0x02		//发生一个IRQ6中断
#define		CMD_SPECIFY			0x03		//设置driver参数
#define		CMD_SENSEI_DRIVE_STATUS		0x04		//useless
#define		CMD_WRITE_DATA			0x05		//写数据， 不直接使用，前面需要设置一些flag
#define		CMD_READ_DATA			0x06		//读数据，不直接使用， 前面需要设置一些flag
#define		CMD_RECALIBRATE			0x07		//把磁头归零
#define		CMD_SENSEI_INTERRUPT		0x08		//在IRQ6中断中获取上次CMD执行结果
#define		CMD_WRITE_DELETED_DATA		0x09		//useless
#define		CMD_READ_ID			0x0A		//生成IRQ6中断
#define		CMD_READ_DELETED_DATA		0x0C		//useless
#define		CMD_FORMAT_TRACK		0x0D		//useless
#define		CMD_SEEK			0x0F		//定位磁头
#define		CMD_VERSION			0x10		//used during init once
#define		CMD_SCAN_EQUAL			0x11		//useless
#define		CMD_PERPENDICULAR_MODE		0x12		//used during initialization, once, maybe
#define		CMD_CONFIGURE			0x13		//设置controller参数
#define		CMD_LOCK			0x14		//protect controller params from a reset
#define		CMD_VERIFY			0x16		//useless
#define		CMD_LOW_OR_EQUAL		0x19		//useless
#define		CMD_HIGH_OR_EQUAL		0x1D		//useless

//option bit
//CMD_READ_DATA/CMD_WRITE_DATA/CMD_FORMAT_TRACK/CMD_VERIFY会用到
//主要是READ/WRITE使用
#define		CMD_BIT_MT			0x80		//Multitrack mode. The controller will switch automatically from Head 0 to Head 1 at the end of the track. This allows you to read/write twice as much data with a single command.
#define		CMD_BIT_MF			0x40		//"MFM" magnetic encoding mode. Always set it for read/write/format/verify operations.
#define		CMD_BIT_SK			0x20		//Skip mode. Ignore this bit and leave it cleared, unless you have a really good reason not to.

//这里设置程序中用到大READ/WRITE CMD（也就是加上CMD_BIT_*)
#define		CMD_READ			CMD_BIT_MT | CMD_BIT_MF | CMD_BIT_SK | CMD_READ_DATA		//MT=MF=SK=1
#define		CMD_WRITE			CMD_BIT_MT | CMD_BIT_MF | CMD_WRITE_DATA			//MT=MF=1		
//DEFINE FDC CMD END

//DMA commands
#define			DMA_READ			0x46		//DMA 读盘，DMA方式字(送DMA端口12,11)
#define			DMA_WRITE			0x4A		//DMA写盘，DMA方式					

#define 		MAX_REPLIES			7		//FDC返回最大字节数
#endif
