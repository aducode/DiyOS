#ifndef _DIYOS_SYSTEM_H
#define _DIYOS_SYSTEM_H
#define cli() __asm__("cli"::)
#define sti() __asm__("sti"::)
#define nop() __asm__("nop"::)
#endif
