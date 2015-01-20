#include "kernel.h"
/**
 * 初始化全局描述符表
 */
static void init_gdt();
void kmain(){
	clean(0,0,25,80);
	disp_str("hello kernel\nI can display some string in screen :)\n",0,0);
	init_gdt();
	//clean(2,0,2,5);
	while(1){
		_hlt();
	}
}
void init_gdt(){
	unsigned short int length = (unsigned short int)(MAX_GDT_ITEMS -1 ) * 8;
	_lgdt(length, gdt);
}


