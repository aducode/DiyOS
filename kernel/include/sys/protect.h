#include "type.h"
#ifndef _PROTECT_H
#define _PROTECT_H

//全局的宏
#define MAX_GDT_ITEMS	32	//最多有32个描述符

//定义gdt ldt idt等
/**
 * 段描述符
 */
struct descriptor	/*共8个字节*/
{
	u16 limit_low;	/*Limit*/
	u16 base_low;   /*Base*/
	u8  base_mid;   /*Base*/
	//u8  attr1;      /*P(1) DPL(2) DT(1) TYPE(4)*/
	u8  type:4;	//描述符类型
	u8  dt:1;	//1段描述符	0系统描述符或门描述符
	u8  privilege:2;//特权级别
	u8  present:1;  //lgdt指令装载后，该位为1
	//u8  limit_high_attr2; /*G(1) D(1) O(1) AVL(1) LimitHigh(4)*/
	u8  limit_hight:4;
	u8  avl:1;	//avl
	u8  nop:1;	//固定0
	u8  d:1;	//D位
	u8  granularity:1;//0表示段界限粒度为1字节  1表示界限粒度4KB
	u8  base_high;	/*Base*/
};

//descriptor.type 取值宏
//存储段描述符类型
#define DA_DR		0x90    //存在的只读数据段类型值
#define DA_DRW		0x92	//存在的可读写数据段属性值
#define DA_DRWA		0x93	//存在的已访问可读写数据段类型值
#define DA_C		0x98	//存在的只执行代码段属性值
#define DA_CR		0x9A	//存在的可执行可读代码段属性值
#define DA_CCO		0x9C	//存在的只执行一致代码段属性值
#define DA_CCOR		0x9E	//存在的可执行可读一致代码段属性值
//系统段描述符类型
#define	DA_LDT		0x82	//局部描述符表段类型值
#define	DA_TaskGate	0x85	//任务门类型值
#define DA_386TSS	0x89	//可用 386 任务状态段类型值
#define	DA_386CGate	0x8C	//386 调用门类型值
#define DA_386IGate	0x8E	//386 中断门类型值
#define DA_386TGate	0x8F	//386 陷阱门类型值
//
#define DA_16		0x00
#define DA_32		0x01
//
#define DA_LIMIT_1B	0x00
#define	DA_LIMIT_4K	0x01
//DPL
#define DA_DPL0		0x00
#define DA_DPL1		0x01
#define	DA_DPL2		0x02
#define DA_DPL3		0x03
//设置段描述符的宏
#define	SET_DESC(base,limit,type,bit,granulity,dpl) \
	{\
		(limit)&0xFFFF, \
		(base) &0xFFFF, \
		((base)>>16&0xFF),\
		(type)&0xFFFF,\
		1,\
		(dpl)&0xFF,\
		0,\
		((limit)>>16)&0xF,\
		0,\
		0,\
		(bit),\
		(granulity),\
		((base)>>24)&0xFF,\
	}
	
/**
 * 描述符表
 */
//struct descriptor_table
//{
//	u16			nop;	/*4Byte对齐*/
//	u16			length;	/*全局表述符表长度 2Byte*/
//	struct descriptor	*ptr;	/*开始指针 4Byte*/	
//};
//gdtPtr need 6Byte but in clang it need align 8Byte
//so we can use char [6] to instread this struct
#endif
