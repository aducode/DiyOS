#ifndef _DIYOS_KLIB_H
#define _DIYOS_KLIB_H
//klib.asm中的汇编函数
extern void _hlt();
extern void _clean(int top, int left, int bottom, int right);

//color
#define COLOR_GREEN		0x02
#define COLOR_RED		0x04
#define COLOR_YELLOW		0x06
#define COLOR_LIGHT_WHITE	0x07
#define	COLOR_WHITE		0x0F
#define COLOR_SYS_ERROR		0x74
extern void _disp_str(char * msg, int row, int column, u8 color);
extern void * _memcpy(void * dest, void * src, int size);
extern void _memset(void * dest, char ch, int size);
extern void _out_byte(u16 port, u8 value);
extern u8 _in_byte(u16 port);
extern void _disable_irq(int irq_no);
extern void _enable_irq(int irq_no);
#endif
