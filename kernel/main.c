extern void _hlt();
extern void clean(int top, int left, int bottom, int right);
extern void disp_str(char * msg, int row, int column);
void kmain(){
	clean(0,0,25,80);
	disp_str("hello kernel\nI can display some string in screen :)\n",2,0);
	clean(2,0,2,5);
	_hlt();
}
