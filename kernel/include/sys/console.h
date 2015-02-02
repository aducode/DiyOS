#include "type.h"
#ifndef _DIYOS_CONSOLE_H
#define _DIYOS_CONSOLE_H
/**
 * console
 */
struct console{
        unsigned int current_start_addr;        //当前显示到了什么位置
        unsigned int original_addr;             //当前控制台对应显存位置
        unsigned int v_mem_limit;               //当前控制台占的显存大小
        unsigned int cursor;                    //光标位置
};
extern int is_current_console(struct console *p_console);
#endif
