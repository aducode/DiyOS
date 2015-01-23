#include "kernel.h"
#include "protect.h"
//异常处理函数
static int cur_row=0;
void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags)
{
	char * err_msg[] = {"#DE Divide Error",
			    "#DB RESERVED",
			    "--  NMI Interrupt",
			    "#BP Breakpoint",
			    "#OF Overflow",
			    "#BR BOUND Range Exceeded",
			    "#UD Invalid Opcode (Undefined Opcode)",
			    "#NM Device Not Available (No Math Coprocessor)",
			    "#DF Double Fault",
			    "    Coprocessor Segment Overrun (reserved)",
			    "#TS Invalid TSS",
			    "#NP Segment Not Present",
			    "#SS Stack-Segment Fault",
			    "#GP General Protection",
			    "#PF Page Fault",
			    "--  (Intel reserved. Do not use.)",
			    "#MF x87 FPU Floating-Point Error (Math Fault)",
			    "#AC Alignment Check",
			    "#MC Machine Check",
			    "#XF SIMD Floating-Point Exception"
	};
	_clean(0,0,25,80);
	_disp_str("System Error    :(    Message is:\n",2,0,COLOR_SYS_ERROR);
	_disp_str(err_msg[vec_no],3,0, COLOR_SYS_ERROR);		
}

//硬件中断处理函数
void default_irq_handler(int irq_no)
{	
	char *irq_msg[] = {
		"irq0 the clock",
		"irq1 keyboard",
		"irq2 cascade!",
		"irq3 secode serial",
		"irq4 first serial",
		"irq5 XT winchester",
		"irq6 floppy",
		"irq7 printer",
		"irq8 realtime clock",
		"irq9 redirected",
		"irq10",
		"irq11",
		"irq12",
		"irq13 FPU exception",
		"irq14 AT winchester",
		"irq15"
	};
	_clean(0,0,25,80);
	_disp_str(irq_msg[irq_no],cur_row,0,COLOR_WHITE);
	cur_row+=1;
	if(cur_row>24) cur_row=0;
}
