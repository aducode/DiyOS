extern void _hlt();
extern void clean(int top, int left, int bottom, int right);
extern void disp_str(char * msg, int row, int column);
void kmain(){
	clean(0,0,4,80);
	disp_str("hello kernel\n",2,0);
	_hlt();
}
