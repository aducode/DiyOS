#include "console.h"
#include "tty.h"
#include "global.h"
int is_current_console(struct console *p_console)
{
	return (p_console == &console_table[current_console]);
}
