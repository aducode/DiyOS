#include "type.h"
#ifndef _DIYOS_FLOPPY_H
#define _DIYOS_FLOPPY_H
/**
 * @function init_floppy
 * @brief 初始化floppy 中断
 * @return
 */
extern void init_floppy();

/**
 * @function floppy_open
 * @brief 打开设备
 * @param device 设备号
 * @return
 */
extern void floppy_open(int device);

/**
 * @function floppy_close
 * @brief 关闭设备
 * @param device 设备号
 * @return
 */
extern void floppy_close(int device);

/**
 * @function floppy_rdwt
 * @brief 设备读写数据
 * @param msg 消息
 * @return
 */
extern void floppy_rdwt(struct message *msg);

/**
 * @function floppy_ioctl
 * @brief 控制设备
 * @param msg 消息
 */
extern void floppy_ioctl(struct message *msg);

//IO port
//I/O address			Read or Write			Register
//0x3f2					Write					DOR: Digital Output Register
//0x3f4					Read					FDC Status: Floppy Disk Status Register
//0x3f5					Read/Write				FDC Data: Floppy Disk Data Register
//0x3f7					Read					DIR: Digital Input Register
//						Write					DCR: Disk Control Register

#define		REG_DOR			0x3f2
#define		FDC_STATUS		0x3f4
#define		FDC_DATA		0x3f5

//DOR 数字输出寄存器
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
//位			Name		Description
//7			RQM			Data ready: FDD ready
//6			DIO			Direction: 1 - FDD to CPU; 0 – CPU to FDD
//5			NDM			DMA set: 1-not DMA; 0-DMA
//4			CB			Controller busy
//3			DDB			Driver D busy
//2			DCB			Driver C busy
//1			DBB			Driver B busy
//0			DAB			Driver A busy
//
// FDC Data：FDC数据寄存器
//FDC Data寄存器用于向FDC发送控制命令或从FDC读取状态，实现数据读写等。FDC的使用比较复杂，可支持多种命令。每个命令都通过一个命令序列实现：命令阶段、执行阶段和结果阶段。
//(1)重新校正命令（FD_RECALIBRATE）
//FD_RECALIBRATE cmd:
//	7	6	5	4	3	2	1	0
//
//	0	0	0	0	0	1	1	1			0x07
//	0	0	0	0	0	0	US1	US2
//结果 磁头移动到track0
#define		CMD_FD_RECALIBRATE				0x07
//(2)磁头寻道命令（FD_SEEK）
//把磁头定位到指定位置，在读写前执行
//FD_SEEK cmd:
//	7	6	5	4	3	2	1	0
//
//	0	0	0	0	1	1	1	1			0x0F
//	0	0	0	0	0	HD	US1	US2			磁头(HD)号驱动器号
//	C										磁道号
//结果 把磁头定位到指定位置，在读写前执行
#define		CMD_FD_SEEK						0x0F

//(3)读扇区数据命令（FD_READ）
//FD_READ cmd:
//	7	6	5	4	3	2	1	0
//
//	MT	MF	SK	0	0	1	1	0			0xE6(MT=MF=SK=1) MT 多磁道操作1表示允许多磁道操作 MF:记录方式1表示选用MFM记录方式，否则FM记录方式 SK:是否跳过有删除标志的扇区1表示跳过
//	0	0	0	0	0	0	US1	US2			驱动器号
//	C										磁道号track
//	H										磁头号head
//	R										起始扇区号 start sector
//	N										扇区字节数
//	EOT										磁道最大扇区号
//	GPL										扇区间隔长度(3)
//	DTL										N=0时，指定扇区字节数
//结果 从软盘读取扇区 返回状态
//	ST0										状态字节0
//	ST1										状态字节1
//	ST2										状态字节2
//	C										磁道号track
//	H										磁头号head
//	R										起始扇区号
//	N										扇区字节数
//
//ST0:
//	7
//	6		ST0_INTR			中断原因。00-正常结束；01-异常结束；10-命令无效；11-软盘驱动器状态改变
//	5		ST0_SE				寻道操作或重新校正操作结束（seek end）
//	4		ST0_ECE				设备检查错误（0磁道校正错误）（Equip. Check Error）
//	3		ST0_NR				软盘未就绪（Not Ready）
//	2		ST0_HA				磁头地址。中断时磁头地址（Head Address）
//	1
//	0		ST0_DS				驱动器号（Driver Select）
//ST1:
//	7		ST1_EOC				范文超过磁道最大扇区号（End of Cylinder）
//	6		Reserve				预留位
//	5		ST1_CRC				CRC校验出错
//	4		ST1_OR				数据传输超时（Over Run）
//	3		Reserve				预留位
//	2		ST1_ND				未找到制定扇区（No Data）
//	1		ST1_WP				写保护（Write Protect）
//	0		ST1_MAM				未找到扇区地址标志ID（Miss Address Mask）
//ST2:
//	7		Reserve				预留位
//	6		ST2_CM				SK=0时，读数据遇到删除标志（Control Mark）
//	5		ST2_CRC				CRC校验出错
//	4		ST2_WC				扇区ID信息的磁道号C不不符（Wrong Cylinder）
//	3		ST2_SEH				检索条件满足（Scan Equal Hit）
//	2		ST2_SNS				检索条件不满足（Scan Not Satisfied）
//	1		ST2_BC				磁道坏（Bad Cylinder）
//	0		ST2_MAM				未找到扇区地址标志ID（Miss Address Mask）
#define		CMD_FD_READ			0xE6	

//(4)写扇区数据命令（FD_WRITE）
//FD_WRITE cmd:
//	7	6	5	4	3	2	1	0
//
//	MT	MF	0	0	0	1	0	1			0xC5(MT=MF=1)
//	0	0	0	0	0	0	US1	US2			驱动器号
//	C										磁道号track
//	H										磁头号head
//	R										起始扇区号 start sector
//	N										扇区字节数
//	EOT										磁道最大扇区号
//	GPL										扇区间隔长度(3)
//	DTL										N=0时，指定扇区字节数
//结果 向软盘写入扇区 返回状态
//	ST0										状态字节0
//	ST1										状态字节1
//	ST2										状态字节2
#define		CMD_FD_WRITE					0xC5

//(5) 检测中断状态命令（FD_SENSEI）
//FD_SENSEI cmd:
//	7	6	5	4	3	2	1	0
//
//	0	0	0	0	1	0	0	0			0x08
//结果
//	ST0										状态字节0
//	C										磁头所在磁道号
#define		CMD_FD_SENSEI					0x08

//(6)设定驱动器参数命令（FD_SPECIFY）
//FD_SPECIFY cmd:
//	7	6	5	4	3	2	1	0
//
//	0	0	0	0	0	0	1	1			0x03
//	|	SRT		|		HUT		|			马达速度(2ms) 磁头卸载时间(32ms)
//	|		HLT				|	ND			磁头加载时间(4ms) 非DMA模式
//结果 设置控制器
#define		CMD_FD_SPECIFY					0x03

//DIR:数字输入寄存器
//DIR寄存器只有D7位有效，用于表示软盘更换状态，其余用于硬盘控制器。

// DCR：磁盘控制寄存器
//DCR仅是用户D0与D1位，用于表示数据传输率。
//00-500kpbs, 01-300kpbs, 10-250kpbs					
#endif
