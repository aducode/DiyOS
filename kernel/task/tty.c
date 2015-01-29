#include "keyboard.h"
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
