#ifndef _DIYOS_KEYBOARD_H
#define _DIYOS_KEYBOARD_H
/** 8042寄存器 **/
#define IO_8042_PORT	0x60
#define CTL_8042_PORT	0x64
extern void init_keyboard();
#endif
