#include "type.h"
#include "interrupt.h"
#ifndef _DIYOS_GLOBAL_H
#define _DIYOS_GLOBAL_H
#ifndef _DIYOS_GLOBAL_C_HERE
//extern struct descriptor_table gdt_ptr;
extern struct descriptor gdt[];
extern u8 gdt_ptr[];
extern struct gate idt[];
extern u8  idt_ptr[];

extern irq_handler irq_handler_table[];

extern struct process* p_proc_ready;
extern struct process  proc_table[];

extern struct tss g_tss;

extern int k_reenter;
#endif
#endif
