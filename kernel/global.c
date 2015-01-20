#include "type.h"	//平台无关的基本数据类型
#include "protect.h"	//保护模式下gdt ldt等数据结构
#define _GLOABL_C_HERE
#include "global.h"
/**
 * 定义全局描述符
 */
struct descriptor gdt[MAX_GDT_ITEMS]={
	SET_DESC(0,0,0,0,0,0),					//Base
	SET_DESC(0x0000LU,0xFFFFFLU,DA_CR,DA_32,DA_LIMIT_4K,DA_DPL0), //0-4G  code segment
	SET_DESC(0x0000LU,0xFFFFFLU,DA_DRW,DA_32,DA_LIMIT_4K,DA_DPL0), //0-4G data segment
	SET_DESC(0xB8000LU,0xFFFFLU,DA_DRW,DA_16,DA_LIMIT_1B,DA_DPL3), //0xB8000-0xC7FFF  for gs
};	//全局描述符表
/*
 *  kernel的全局变量
 */
/*
struct descriptor_table gdt_ptr={
//	0, // 对齐
	(MAX_GDT_ITEMS-1)*8 ,	//gdt长度
	gdt,		//gdt指针
} a
;
*/	//全局描述符表
