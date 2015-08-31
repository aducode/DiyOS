#include "type.h"
#ifndef _DIYOS_FAT12_H
#define _DIYOS_FAT12_H
/**
 * @struct BPB
 * @brief FAT12 Boot Parameter Block
 * BPB位于引导扇区偏移11Byte处
 *		之前有:
 *			BS_jmpBOOT		3Byte
 *			BS_OEMName		8Byte
 */
struct BPB {
	u16 bytes_per_sec;		//每扇区字节数  
	u8  sec_per_clus;		//每簇扇区数  
	u16 rsvd_sec_cnt;		//Boot记录占用的扇区数  
	u8  num_fats;			//FAT表个数  
	u16 root_ent_cnt;		//根目录最大文件数  
	u16 tot_sec16;			//扇区总数
	u8  media;				//介质描述符
	u16 fat_sz16;			//每FAT扇区数  
	u16 sec_per_trk;		//每磁道扇区数
	u16 num_heads;			//磁头数
	u32 hidd_sec;			//隐藏扇区数
	u32 tot_sec32;			//如果BPB_FATSz16为0，该值为FAT扇区数  
};

/**
 * @function dump_bpb
 * @brief dump出bpb的值
 */
extern void dump_bpb(struct BPB * bpb_ptr);
#endif