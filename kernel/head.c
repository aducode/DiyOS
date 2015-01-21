#include "type.h"
#include "kernel.h"
#include "global.h"
#include "protect.h"

void head()
{
	//调用之前gdt_ptr中已经加载进gdt寄存器的值了
        _disp_str("Reset GDT now ...\n", 10, 0);
	//将loader中的gdt复制到信的gdt中
	_memcpy(&gdt, 
		(void*)(*((u32*)(&gdt_ptr[2]))),
		*((u16*)(&gdt_ptr[0]))+1
	);		
	u16 * p_gdt_limit = (u16*)(&gdt_ptr[0]);
	u32 * p_gdt_base = (u32*)(&gdt_ptr[2]);
	//给gdt_ptr设置新的值
	*p_gdt_limit = MAX_GDT_ITEMS * sizeof(struct descriptor) - 1;
	//*p_gdt_base = (u32)&gdt;
	*p_gdt_base =(u32)gdt;
}
