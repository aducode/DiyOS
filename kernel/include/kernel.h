#ifndef _KERNEL_H
#define _KERNEL_H
extern void _hlt();
extern void clean(int top, int left, int bottom, int right);
extern void disp_str(char * msg, int row, int column);
#endif
