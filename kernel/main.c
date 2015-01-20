#include "kernel.h"
void kmain(){
	clean(0,0,25,80);
	disp_str("hello kernel\nI can display some string in screen :)\n",0,0);
	//clean(2,0,2,5);
	_hlt();
}
