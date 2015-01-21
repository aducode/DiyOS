//定义保护模式下使用的一些数据结构
//保护模式下的代码段
#include "type.h"
#ifndef _DIYOS_PROTECT_H
#define _DIYOS_PROTECT_H

//GDT个数
#define MAX_GDT_ITEMS	128	//最多有128个全局描述符 可用的有127个 0作为base
#define MAX_IDT_ITEMS	256	//做多有256个中断描述符表

//8259A端口
#define INT_M_CTL	0x20	//I/O port for interrupt cotroller <Master>
#define INT_M_CTLMASK	0x21	//setting bits in this port disables ints <Master>
#define INT_S_CTL	0xA0	//I/O port for second interrupt controller <Slave>
#define INT_S_CTLMASK	0xA1	//setting bits in this port disables ints <Slave>

#define EOI		0x20

/* 中断向量 */
#define	INT_VECTOR_DIVIDE		0x0
#define	INT_VECTOR_DEBUG		0x1
#define	INT_VECTOR_NMI			0x2
#define	INT_VECTOR_BREAKPOINT		0x3
#define	INT_VECTOR_OVERFLOW		0x4
#define	INT_VECTOR_BOUNDS		0x5
#define	INT_VECTOR_INVAL_OP		0x6
#define	INT_VECTOR_COPROC_NOT		0x7
#define	INT_VECTOR_DOUBLE_FAULT		0x8
#define	INT_VECTOR_COPROC_SEG		0x9
#define	INT_VECTOR_INVAL_TSS		0xA
#define	INT_VECTOR_SEG_NOT		0xB
#define	INT_VECTOR_STACK_FAULT		0xC
#define	INT_VECTOR_PROTECTION		0xD
#define	INT_VECTOR_PAGE_FAULT		0xE
#define	INT_VECTOR_COPROC_ERR		0x10

/* 中断向量 */
#define	INT_VECTOR_IRQ0			0x20
#define	INT_VECTOR_IRQ8			0x28

//定义gdt ldt idt等
/**
 * 段描述符
 * 放弃使用位域了，参考《一个操作系统的实现》的代码
 */
struct descriptor	/*共8个字节*/
{
	u16 limit_low;	/*Limit*/
	u16 base_low;   /*Base*/
	u8  base_mid;   /*Base*/
	u8  attr1;      /*P(1) DPL(2) DT(1) TYPE(4)*/
	//小端模式下，低地址先放高位域成员
	//所以顺序应该改变
	/*
	u8  type:4;	//描述符类型
	u8  dt:1;	//1段描述符	0系统描述符或门描述符
	u8  privilege:2;//特权级别
	u8  present:1;  //lgdt指令装载后，该位为1
	*/
	/*
	u8  present:1;
	u8  privilege:2;
	u8  dt:1;
	u8  type:4;
	*/
	u8  limit_high_attr2; /*G(1) D(1) O(1) AVL(1) LimitHigh(4)*/
	/*
	u8  limit_hight:4;
	u8  avl:1;	//avl
	u8  nop:1;	//固定0
	u8  d:1;	//D位
	u8  granularity:1;//0表示段界限粒度为1字节  1表示界限粒度4KB
	*/
	//
	/*
	u8  granularity:1;
	u8  d:1;
	u8  nop:1;
	u8  avl:1;
	u8  limit_hight:4;
	*/
	u8  base_high;	/*Base*/
};
//门描述符
struct gate{
	u16	offset_low;	/* Offset Low */
	u16	selector;	/* Selector */
	u8	dcount;		/* 该字段只在调用门描述符中有效。如果在利用
				   调用门调用子程序时引起特权级的转换和堆栈
				   的改变，需要将外层堆栈中的参数复制到内层
				   堆栈。该双字计数字段就是用于说明这种情况
				   发生时，要复制的双字参数的数量。*/
	u8	attr;		/* P(1) DPL(2) DT(1) TYPE(4) */
	u16	offset_high;	/* Offset High */
};
/* GDT */
/* 描述符索引 */
#define	INDEX_DUMMY		0	// ┓
#define	INDEX_FLAT_C		1	// ┣ LOADER 里面已经确定了的.
#define	INDEX_FLAT_RW		2	// ┃
#define	INDEX_VIDEO		3	// ┛
/* 选择子 */
#define	SELECTOR_DUMMY		   0		// ┓
#define	SELECTOR_FLAT_C		0x08		// ┣ LOADER 里面已经确定了的.
#define	SELECTOR_FLAT_RW	0x10		// ┃
#define	SELECTOR_VIDEO		(0x18+3)	// ┛<-- RPL=3

#define	SELECTOR_KERNEL_CS	SELECTOR_FLAT_C
#define	SELECTOR_KERNEL_DS	SELECTOR_FLAT_RW


/* 描述符类型值说明 */
#define	DA_32			0x4000	/* 32 位段				*/
#define	DA_LIMIT_4K		0x8000	/* 段界限粒度为 4K 字节			*/
#define	DA_DPL0			0x00	/* DPL = 0				*/
#define	DA_DPL1			0x20	/* DPL = 1				*/
#define	DA_DPL2			0x40	/* DPL = 2				*/
#define	DA_DPL3			0x60	/* DPL = 3				*/
/* 存储段描述符类型值说明 */
#define	DA_DR			0x90	/* 存在的只读数据段类型值		*/
#define	DA_DRW			0x92	/* 存在的可读写数据段属性值		*/
#define	DA_DRWA			0x93	/* 存在的已访问可读写数据段类型值	*/
#define	DA_C			0x98	/* 存在的只执行代码段属性值		*/
#define	DA_CR			0x9A	/* 存在的可执行可读代码段属性值		*/
#define	DA_CCO			0x9C	/* 存在的只执行一致代码段属性值		*/
#define	DA_CCOR			0x9E	/* 存在的可执行可读一致代码段属性值	*/
/* 系统段描述符类型值说明 */
#define	DA_LDT			0x82	/* 局部描述符表段类型值			*/
#define	DA_TaskGate		0x85	/* 任务门类型值				*/
#define	DA_386TSS		0x89	/* 可用 386 任务状态段类型值		*/
#define	DA_386CGate		0x8C	/* 386 调用门类型值			*/
#define	DA_386IGate		0x8E	/* 386 中断门类型值			*/
#define	DA_386TGate		0x8F	/* 386 陷阱门类型值			*/


// 系统级别
//R0
#define	PRIVILEGE_KERNEL	0	//内核级别
#define PRIVILEGE_USER		3	//用户级别
#endif
