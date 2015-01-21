#include "kernel.h"
//异常处理函数
void exception_handler(int vec_no, int err_code, int eip, int cs, int eflags)
{
	int i;
//int text_color = 0x74;
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
