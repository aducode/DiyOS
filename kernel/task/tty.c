#include "keyboard.h"
#include "klib.h"
/**
 * tty进程
 */
void task_tty()
{
	while(1)
	{
		keyboard_read();
	}
}


/**
 *处理键盘码，显示到屏幕
 */
void show(u8 key){
	char output[2] = {'\0','\0'};
	if(!(key& FLAG_EXT)){
		output[0] = key & 0xFF;
		_disp_str(output,11,0,COLOR_WHITE);
	}	
}
