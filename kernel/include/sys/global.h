#include "type.h"
#ifndef _DIYOS_GLOBAL_H
#define _DIYOS_GLOBAL_H
#ifndef _DIYOS_GLOBAL_C_HERE
//extern struct descriptor_table gdt_ptr;
extern struct descriptor gdt[];
extern u8 gdt_ptr[];
extern struct gate idt[];
extern u8  idt_ptr[];
#endif
#endif
