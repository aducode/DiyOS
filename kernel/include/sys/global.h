#include "type.h"
#ifndef _GLOBAL_H
#define _GLOBAL_H
#ifndef _GLOBAL_C_HERE
//extern struct descriptor_table gdt_ptr;
extern struct descriptor gdt[];
extern u8 gdt_ptr[];
#endif
#endif
