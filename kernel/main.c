#include "kernel.h"
/**
 * 初始化全局描述符表
 */
void kmain(){
//	_clean(0,0,25,80);
	_disp_str("hello kernel\nI can display some string in screen :)\n",15,0,COLOR_LIGHT_WHITE);
	//启动进程，开中断
//	_restart();
	while(1){
		_hlt();
	}
}


