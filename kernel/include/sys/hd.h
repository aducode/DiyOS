#include "type.h"
#ifndef _DIYOS_HD_H
#define _DIYOS_HD_H
//hard disk sector
#define	SECTOR_SIZE	512
#define SECTOR_BITS	(SECTOR_SIZE * 8)
#define SECTOR_SIZE_SHIFT	9
//硬盘
/********************************************/
/* I/O Ports used by hard disk controllers. */
/********************************************/
/* slave disk not supported yet, all master registers below */

/* Command Block Registers */
/*	MACRO		PORT			DESCRIPTION			INPUT/OUTPUT	*/
/*	-----		----			-----------			------------	*/
#define REG_DATA	0x1F0		/*	Data				I/O		*/
#define REG_FEATURES	0x1F1		/*	Features			O		*/
#define REG_ERROR	REG_FEATURES	/*	Error				I		*/
					/* 	The contents of this register are valid only when the error bit
						(ERR) in the Status Register is set, except at drive power-up or at the
						completion of the drive's internal diagnostics, when the register
						contains a status code.
						When the error bit (ERR) is set, Error Register bits are interpreted as such:
						|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						| BRK | UNC |     | IDNF|     | ABRT|TKONF| AMNF|
						+-----+-----+-----+-----+-----+-----+-----+-----+
						   |     |     |     |     |     |     |     |
						   |     |     |     |     |     |     |     `--- 0. Data address mark not found after correct ID field found
						   |     |     |     |     |     |     `--------- 1. Track 0 not found during execution of Recalibrate command
						   |     |     |     |     |     `--------------- 2. Command aborted due to drive status error or invalid command
						   |     |     |     |     `--------------------- 3. Not used
						   |     |     |     `--------------------------- 4. Requested sector's ID field not found
						   |     |     `--------------------------------- 5. Not used
						   |     `--------------------------------------- 6. Uncorrectable data error encountered
						   `--------------------------------------------- 7. Bad block mark detected in the requested sector's ID field
					*/
#define REG_NSECTOR	0x1F2		/*	Sector Count			I/O		*/
#define REG_LBA_LOW	0x1F3		/*	Sector Number / LBA Bits 0-7	I/O		*/
#define REG_LBA_MID	0x1F4		/*	Cylinder Low / LBA Bits 8-15	I/O		*/
#define REG_LBA_HIGH	0x1F5		/*	Cylinder High / LBA Bits 16-23	I/O		*/
#define REG_DEVICE	0x1F6		/*	Drive | Head | LBA bits 24-27	I/O		*/
					/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						|  1  |  L  |  1  | DRV | HS3 | HS2 | HS1 | HS0 |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						         |           |   \_____________________/
						         |           |              |
						         |           |              `------------ If L=0, Head Select.
						         |           |                                    These four bits select the head number.
						         |           |                                    HS0 is the least significant.
						         |           |                            If L=1, HS0 through HS3 contain bit 24-27 of the LBA.
						         |           `--------------------------- Drive. When DRV=0, drive 0 (master) is selected. 
						         |                                               When DRV=1, drive 1 (slave) is selected.
						         `--------------------------------------- LBA mode. This bit selects the mode of operation.
					 	                                                            When L=0, addressing is by 'CHS' mode.
					 	                                                            When L=1, addressing is by 'LBA' mode.
					*/
#define REG_STATUS	0x1F7		/*	Status				I		*/
					/* 	Any pending interrupt is cleared whenever this register is read.
						|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						| BSY | DRDY|DF/SE|  #  | DRQ |     |     | ERR |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						   |     |     |     |     |     |     |     |
						   |     |     |     |     |     |     |     `--- 0. Error.(an error occurred)
						   |     |     |     |     |     |     `--------- 1. Obsolete.
						   |     |     |     |     |     `--------------- 2. Obsolete.
						   |     |     |     |     `--------------------- 3. Data Request. (ready to transfer data)
						   |     |     |     `--------------------------- 4. Command dependent. (formerly DSC bit)
						   |     |     `--------------------------------- 5. Device Fault / Stream Error.
						   |     `--------------------------------------- 6. Drive Ready.
						   `--------------------------------------------- 7. Busy. If BSY=1, no other bits in the register are valid.
					*/
#define	STATUS_BSY	0x80
#define	STATUS_DRDY	0x40
#define	STATUS_DFSE	0x20
#define	STATUS_DSC	0x10
#define	STATUS_DRQ	0x08
#define	STATUS_CORR	0x04
#define	STATUS_IDX	0x02
#define	STATUS_ERR	0x01

#define REG_CMD		REG_STATUS	/*	Command				O		*/
					/*
						+--------+---------------------------------+-----------------+
						| Command| Command Description             | Parameters Used |
						| Code   |                                 | PC SC SN CY DH  |
						+--------+---------------------------------+-----------------+
						| ECh  @ | Identify Drive                  |             D   |
						| 91h    | Initialize Drive Parameters     |    V        V   |
						| 20h    | Read Sectors With Retry         |    V  V  V  V   |
						| E8h  @ | Write Buffer                    |             D   |
						+--------+---------------------------------+-----------------+
					
						KEY FOR SYMBOLS IN THE TABLE:
						===========================================-----=========================================================================
						PC    Register 1F1: Write Precompensation	@     These commands are optional and may not be supported by some drives.
						SC    Register 1F2: Sector Count		D     Only DRIVE parameter is valid, HEAD parameter is ignored.
						SN    Register 1F3: Sector Number		D+    Both drives execute this command regardless of the DRIVE parameter.
						CY    Register 1F4+1F5: Cylinder low + high	V     Indicates that the register contains a valid paramterer.
						DH    Register 1F6: Drive / Head
					*/

/* Control Block Registers */
/*	MACRO		PORT			DESCRIPTION			INPUT/OUTPUT	*/
/*	-----		----			-----------			------------	*/
#define REG_DEV_CTRL	0x3F6		/*	Device Control			O		*/
					/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						| HOB |  -  |  -  |  -  |  -  |SRST |-IEN |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						   |                             |     |
						   |                             |     `--------- Interrupt Enable.
						   |                             |                  - IEN=0, and the drive is selected,
						   |                             |                    drive interrupts to the host will be enabled.
						   |                             |                  - IEN=1, or the drive is not selected,
						   |                             |                    drive interrupts to the host will be disabled.
						   |                             `--------------- Software Reset.
						   |                                                - The drive is held reset when RST=1.
						   |                                                  Setting RST=0 re-enables the drive.
						   |                                                - The host must set RST=1 and wait for at least
						   |                                                  5 microsecondsbefore setting RST=0, to ensure
						   |                                                  that the drive recognizes the reset.
						   `--------------------------------------------- HOB (High Order Byte)
						                                                    - defined by 48-bit Address feature set.
					*/
#define REG_ALT_STATUS	REG_DEV_CTRL	/*	Alternate Status		I		*/
					/*	This register contains the same information as the Status Register.
						The only difference is that reading this register does not imply interrupt acknowledge or clear a pending interrupt.
					*/

#define REG_DRV_ADDR	0x3F7		/*	Drive Address			I		*/

/***************/
/* DEFINITIONS */
/***************/
#define	HD_TIMEOUT		10000	/* in millisec */
#define	PARTITION_TABLE_OFFSET	0x1BE
#define ATA_IDENTIFY		0xEC
#define ATA_READ		0x20
#define ATA_WRITE		0x30
//
#define MAKE_DEVICE_REG(lba, drv, lba_highest) (((lba)<<6) | \
					       ((drv)<<4) | \
					       (lba_highest & 0xF)|0xA0)
/**
 * @struct hd_cmd
 * @brief 与硬盘寄存器操作的命结构体
 */
struct hd_cmd{
	u8	features;
	u8	count;
	u8	lba_low;
	u8	lba_mid;
	u8	lba_high;
	u8	device;
	u8	command;
};

extern void init_hd();

//设备相关宏
//系统最多支持两块硬盘
#define MAX_DRIVES	2
//每块硬盘支持的最大主分区或扩展分区数为4
#define PART_PER_DRIVE	4
//每个扩展分区最大逻辑分区数
#define SUB_PER_PART	16
//每块硬盘的最大逻辑分区数
#define SUB_PER_DRIVE	(SUB_PER_PART * PART_PER_DRIVE)
//每块硬盘的主分区数（hd0-4 第一块硬盘 其中hd0表示整体）（hd5-9表示第二块硬盘 hd5表示整体）
#define PRIM_PER_DRIVE	(PART_PER_DRIVE + 1)
//系统支持的最大主分区数
/**
 *@def MAX_PRIM
 *Defines the max minor number of the primary partitions.
 * If there are 2 disks, prim_dev ranges in hd[0-9], this macro will eqauls 9.
 * 最大主分区id号 9
 * hd0表示第一块硬盘整体 hd1-4表示第一块硬盘上的4个主分区（扩展分区）
 * hd5表示第二块硬盘整体 hd6-9表示第二块硬盘上的4个主分区（扩展分区）
 */
#define MAX_PRIM	(MAX_DRIVES * PRIM_PER_DRIVE - 1)
//系统支持的最大逻辑分区数
#define MAX_SUBPARTITIONS	(SUB_PER_DRIVE * MAX_DRIVES)
//
//主设备号
#define NO_DEV          0
#define DEV_FLOPPY      1
#define DEV_CDROM       2
#define DEV_HD          3
#define DEV_CHAR_TTY    4
#define DEV_SCSI        5

//设备号与驱动程序pid映射关系在global.c中指定
//make device number from major and minor numbers
#define MAJOR_SHIFT     8
//根据主设备号和次设备号生成设备号
#define MAKE_DEV(a,b)   ((a<<MAJOR_SHIFT)|b)
//separate major and minor numbers from device number
#define MAJOR(x)        ((x>>MAJOR_SHIFT) & 0xFF)
#define MINOR(x)        (x & 0xFF)
//
//硬盘设备号
#define MINOR_hd1	0x01				//主分区
#define MINOR_hd1a      0x10
#define MINOR_hd2a      (MINOR_hd1a + SUB_PER_PART) 
//
#define ROOT_DEV	MAKE_DEV(DEV_HD, MINOR_hd1)    //我们系统的文件系统根目录在主分区（现在只有一个分区）

//我们系统的ROOT_DEV
//分区类型
//没有分区
#define NO_PART		0x00
//扩展分区标示
#define EXT_PART	0x05


#define P_PRIMARY	0
#define P_EXTENDED	1
/**
 * @struct part_ent
 * @brief partition entry 分区表项
 */
struct part_ent {
	u8 boot_ind;		/**
				 * boot indicator
				 *   Bit 7 is the active partition flag,
				 *   bits 6-0 are zero (when not zero this
				 *   byte is also the drive number of the
				 *   drive to boot so the active partition
				 *   is always found on drive 80H, the first
				 *   hard disk).
				 */

	u8 start_head;		/**
				 * Starting Head
				 */

	u8 start_sector;	/**
				 * Starting Sector.
				 *   Only bits 0-5 are used. Bits 6-7 are
				 *   the upper two bits for the Starting
				 *   Cylinder field.
				 */

	u8 start_cyl;		/**
				 * Starting Cylinder.
				 *   This field contains the lower 8 bits
				 *   of the cylinder value. Starting cylinder
				 *   is thus a 10-bit number, with a maximum
				 *   value of 1023.
				 */

	u8 sys_id;		/**
				 * System ID
				 * e.g.
				 *   01: FAT12
				 *   81: MINIX
				 *   83: Linux
				 */

	u8 end_head;		/**
				 * Ending Head
				 */

	u8 end_sector;		/**
				 * Ending Sector.
				 *   Only bits 0-5 are used. Bits 6-7 are
				 *   the upper two bits for the Ending
				 *    Cylinder field.
				 */

	u8 end_cyl;		/**
				 * Ending Cylinder.
				 *   This field contains the lower 8 bits
				 *   of the cylinder value. Ending cylinder
				 *   is thus a 10-bit number, with a maximum
				 *   value of 1023.
				 */

	u32 start_sect;	/**
				 * starting sector counting from
				 * 0 / Relative Sector. / start in LBA
				 */

	u32 nr_sects;		/**
				 * nr of sectors in partition
				 */

};
/**
 * @struct part_info
 * @brief 分区信息
 */
struct part_info
{
	u32 base;	//of sart sector(NOT byte offset, but SECTOR
	u32 size;	//how many sectors in this partition
};
/**
 * @struct hd_info
 * @brief 硬盘设备信息
 */
struct hd_info
{
        int     open_cnt;
        struct part_info        primary[PRIM_PER_DRIVE];
	struct part_info	logical[SUB_PER_DRIVE];
};




//public function
extern void hd_open(int device);
extern void hd_close(int device);
extern void hd_rdwt(struct message *msg);
extern void hd_ioctl(struct message *msg);
//macro for hd_ioctl
#define DIOCTL_GET_GEO	1
#endif
