#include "console.h"
#include "tty.h"
#include "global.h"
#include "klib.h"
static void set_cursor(unsigned int position);
static void set_video_start_addr(u32 addr);
/**
 * 判断是否是当前console
 */
int is_current_console(struct console *p_console)
{
	return (p_console == &console_table[current_console]);
}
/**
 * 输出字符到屏幕
 */
void out_char(struct console *p_console, char ch)
{
	u8 *p_vmem = (u8*)(V_MEM_BASE + p_console->cursor * 2);
	
	*p_vmem++ = ch;
	*p_vmem++ = DEFAULT_CHAR_COLOR;
	p_console->cursor++;
	set_cursor(p_console->cursor);
}

/**
 * 设置光标位置
 */
void set_cursor(unsigned int position)
{
	_disable_int();
	_out_byte(CRTC_ADDR_REG, CURSOR_H);
	_out_byte(CRTC_DATA_REG, (position>>8)&0xFF);
	_out_byte(CRTC_ADDR_REG, CURSOR_L);
	_out_byte(CRTC_DATA_REG, position & 0xFF);
	_enable_int();
}

/**
 *初始化屏幕
 */
void init_screen(struct tty *p_tty)
{
	int nr_tty  =  p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;
	
	int v_mem_size = V_MEM_SIZE >>1;	//显存总大小
	int con_v_mem_size = v_mem_size/CONSOLE_COUNT;
	p_tty->p_console->original_addr = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit=con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;
	//默认光标位置在开始处
	p_tty->p_console->cursor = p_tty->p_console->original_addr;
	out_char(p_tty->p_console, nr_tty+'0');
	out_char(p_tty->p_console,'#');
	set_cursor(p_tty->p_console->cursor);
}

/**
 * 切换控制台
 */
void select_console(int console_idx)
{
	if((console_idx<0)||(console_idx >= CONSOLE_COUNT)){
		return;
	}
	current_console = console_idx;
	set_cursor(console_table[console_idx].cursor);
	set_video_start_addr(console_table[console_idx].current_start_addr);
}

void set_video_start_addr(u32 addr)
{
	_disable_int();
	_out_byte(CRTC_ADDR_REG, START_ADDR_H);
	_out_byte(CRTC_DATA_REG, (addr>>8)&0xFF);
	_out_byte(CRTC_ADDR_REG, START_ADDR_L);
	_out_byte(CRTC_DATA_REG, addr & 0xFF);
	_enable_int();
}
