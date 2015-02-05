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
 *color
 */
#define BLACK   0x0     /* 0000 */
#define WHITE   0x7     /* 0111 */
#define RED     0x4     /* 0100 */
#define GREEN   0x2     /* 0010 */
#define BLUE    0x1     /* 0001 */
#define FLASH   0x80    /* 1000 0000 */
#define BRIGHT  0x08    /* 0000 1000 */
#define	MAKE_COLOR(x,y)	((x<<4) | y) /* MAKE_COLOR(Background,Foreground) */

/**
 * 默认黑底白字
 */
#define DEFAULT_CHAR_COLOR	(MAKE_COLOR(BLACK,WHITE))
//#define DEFAULT_CHAR_COLOR	0x07
#define GRAY_CHAR		(MAKE_COLOR(BLACK, BLACK) | BRIGHT)
#define RED_CHAR		(MAKE_COLOR(BLUE, RED) | BRIGHT)
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
#define SCR_UP	1 //scroll upward
#define SCR_DN -1 //scroll downward

#define SCR_SIZE (80 *25)
#define SCR_WIDTH 80
extern void scroll_screen(struct console *p_console, int direction);
extern void select_console(int console_idx);
#endif
