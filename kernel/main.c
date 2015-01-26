#include "kernel.h"
/**
 * 初始化全局描述符表
 */
void kmain(){
//	_clean(0,0,25,80);
	_disp_str("hello kernel\nI can display some string in screen :)\n",15,0,COLOR_LIGHT_WHITE);
	//设置进程
	struct task * p_task	= task_table;
	struct process * p_proc	= proc_table;
	char * p_task_stack	= task_stack + STACK_SIZE_TOTAL	;//栈低地址增长
	u16 selector_ldt 	= SELECTOR_LDT_FIRST;
	
	int i;
	for(i=0;i<TASKS_COUNT + PROCS_COUNT;i++)
	{
		_strcpy(p_proc->p_name, p_task->name);
		p_proc->pid = i;
		p_proc->ldt_sel = selector_ldt;
		_memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS>>3], sizeof(struct descriptor));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK <<5;
		_memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS>>3],sizeof(struct descriptor));
		p_proc->ldts[1].attr1 = DA_DRW|PRIVILEGE_TASK<<5;
		p_proc->regs.cs = ((8*0)&SA_RPL_MASK & SA_TI_MASK)|SA_TIL|RPL_TASK;
		p_proc->regs.ds = ((8*1)&SA_RPL_MASK & SA_TI_MASK)|SA_TIL|RPL_TASK;
		p_proc->regs.es = ((8*1)&SA_RPL_MASK & SA_TI_MASK)|SA_TIL|RPL_TASK;
		p_proc->regs.fs = ((8*1)&SA_RPL_MASK & SA_TI_MASK)|SA_TIL|RPL_TASK;
		p_proc->regs.ss = ((8*1)&SA_RPL_MASK & SA_TI_MASK)|SA_TIL|RPL_TASK;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK)|RPL_TASK;
		p_proc->regs.eip = (u32)p_task->entry;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202;	//IF=0 IOPL=1
		
		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt+=1<<3;
	}
	p_proc_ready = proc_table;
	//开启时钟中断
	_disable_irq(CLOCK_IRQ);
	irq_handler_table[CLOCK_IRQ] = default_irq_handler;
	_enable_irq(CLOCK_IRQ);
	//TODO 设置进程的ldt
	//启动进程，开中断
	_disp_str("start new process now ...",17,0, COLOR_GREEN);
	_restart();
	while(1){
		_hlt();
	}
}

void testA()
{
	_clean(0,0,25,80);
	_disp_str("testA",17,0, COLOR_WHITE);
}
