#include "type.h"
#ifndef _DIYOS_CONSOLE_H
#define _DIYOS_CONSOLE_H
//VGA显卡
#define	CRTC_ADDR_REG	0x3D4	/* CRT Controller Registers - Addr Register */
#define	CRTC_DATA_REG	0x3D5	/* CRT Controller Registers - Data Register */
#define	START_ADDR_H	0xC	/* reg index of video mem start addr (MSB) */
#define	START_ADDR_L	0xD	/* reg index of video mem start addr (LSB) */
#define	CURSOR_H	0xE	/* reg index of cursor position (MSB) */
#define	CURSOR_L	0xF	/* reg index of cursor position (LSB) */
#define	V_MEM_BASE	0xB8000	/* base of color video memory */
#define	V_MEM_SIZE	0x8000	/* 32K: B8000H -> BFFFFH */

/**
 * 默认黑底白字
 */
#define DEFAULT_CHAR_COLOR	0x07
struct tty;
/**
 * console
 */
struct console{
        unsigned int current_start_addr;        //当前显示到了什么位置
        unsigned int original_addr;             //当前控制台对应显存位置
        unsigned int v_mem_limit;               //当前控制台占的显存大小
        unsigned int cursor;                    //光标位置
};
extern void init_screen(struct tty *p_tty);
extern void out_char(struct console *p_console, char ch);
extern int is_current_console(struct console *p_console);
#endif
