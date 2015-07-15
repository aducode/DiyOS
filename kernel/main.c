#include <elf.h>
#include "kernel.h"

static int get_kernel_map(unsigned int *b, unsigned int *l);
/**
 * 初始化全局描述符表
 */
void kmain(){
//	_clean(0,0,25,80);
	_disp_str("hello kernel\nI can display some string in screen :)\n",15,0,COLOR_LIGHT_WHITE);
	//设置进程
	//任务表中的第一项
	struct task * p_task;//	= task_table;
	//进程表中的第一项
	struct process * p_proc	= proc_table;
	char * p_task_stack	= task_stack + STACK_SIZE_TOTAL	;//栈低地址增长
	//GDT中ldt选择子
	u16 selector_ldt 	= SELECTOR_LDT_FIRST;
	
	u8 privilege;
	u8 rpl;
	int eflags;
	int i,j;
	for(i=0;i<TASKS_COUNT + PROCS_COUNT;i++)
	{
		if(i>=TASKS_COUNT + NATIVE_PROCS_COUNT){
			p_proc->p_flags = FREE_SLOT;
			p_proc++;
			continue;
		}	
		if(i<TASKS_COUNT){
			//ring1
			p_task = task_table + i;
			privilege = PRIVILEGE_TASK;
			rpl = RPL_TASK;
			eflags = 0x1202;
		} else {
			p_task = user_proc_table + (i-TASKS_COUNT);
			privilege = PRIVILEGE_USER;
			rpl = RPL_USER;
			eflags =0x202;
		}
		//把task中的名称复制到进程表项中
		_strcpy(p_proc->name, p_task->name);
		p_proc->p_parent = NO_TASK;
		//设置进程id
		p_proc->pid = i;
		//设置进程的ldt选择子
                p_proc->ldt_sel = selector_ldt;
		if(strcmp(p_proc->name, "INIT") != 0){
			//设置进程的local descriptor table
			//设置ldt第一项为gdt的代码段
			_memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS>>3], sizeof(struct descriptor));
			//重新设置代码段属性值
			p_proc->ldts[0].attr1 = DA_C | privilege <<5;
			//设置ldt第二项为gdt的数据段
			_memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS>>3],sizeof(struct descriptor));
			//重新设置数据段属性
			p_proc->ldts[1].attr1 = DA_DRW| privilege<<5;
		} else {
			//init process
			//设置进程的ldt选择子
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret==0);
			init_desc(&p_proc->ldts[0],
				0,//bytes before the entry point are useless (wasted) for the INIT process, doesn't matter
				(k_base + k_limit) >> LIMIT_4K_SHIFT,
				DA_32|DA_LIMIT_4K|DA_C|privilege<<5);
			init_desc(&p_proc->ldts[1],0,(k_base+k_limit)>>LIMIT_4K_SHIFT, DA_32|DA_LIMIT_4K|DA_DRW|privilege<<5);
		}
		//设置寄存器的值
		p_proc->regs.cs = ((8*0)&SA_RPL_MASK & SA_TI_MASK)|SA_TIL|rpl;
		p_proc->regs.ds = ((8*1)&SA_RPL_MASK & SA_TI_MASK)|SA_TIL|rpl;
		p_proc->regs.es = ((8*1)&SA_RPL_MASK & SA_TI_MASK)|SA_TIL|rpl;
		p_proc->regs.fs = ((8*1)&SA_RPL_MASK & SA_TI_MASK)|SA_TIL|rpl;
		p_proc->regs.ss = ((8*1)&SA_RPL_MASK & SA_TI_MASK)|SA_TIL|rpl;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK)|rpl;
		p_proc->regs.eip = (u32)p_task->entry;
#ifdef	_SHOW_PROC_ENTRY_
		char msg[10];
		static int row = 10;
		itoa(p_proc->regs.eip, msg, 16);
		_disp_str(msg, row++, 0, COLOR_RED);
#endif
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;	//IF=0 IOPL=1
		//指定tty索引，默认都是0
		//p_proc->tty_idx = 0;	
		//设置进程状态
		p_proc->p_flags = 0;
		p_proc->p_msg = 0;
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;
		p_proc->next_sending = 0;
		//优先级 时间片
		p_proc->ticks = p_proc->priority = p_task->priority;
		//
		for(j=0;j<MAX_FILE_COUNT;j++){
			p_proc->filp[j]=0;
		}	
		//下一个进程ldt的值	
		p_task_stack -= p_task->stacksize;
		p_proc++;
		//p_task++;
		selector_ldt+=1<<3;
	}
#ifdef _SHOW_PROC_ENTRY_
	while(1);
#endif
	k_reenter = 0;
	//设置将要开始的进程
	p_proc_ready = proc_table;
	//开启时钟中断
	init_clock();
	//初始化键盘放到tty task里面进行
	//init_keyboard();
//	_disable_irq(CLOCK_IRQ);
//	irq_handler_table[CLOCK_IRQ] = default_irq_handler;
//	_enable_irq(CLOCK_IRQ);
	//启动进程
	_disp_str("start new process now ...",17,0, COLOR_GREEN);
	_clean(0,0,25,80);
	_restart();	//0x1364
	//上面restart之后就会进入到进程里，下面不会被执行
	while(1){
		_disp_str("yoooooooo!",0,0,COLOR_GREEN);
		_hlt();
	}
}

/**
 * @function empty_proc
 * @brief 这是一个空进程体，什么都不操作，只是为了在所有进程都阻塞的时候，进程调度不限入死循环，导致时钟中断不会返回
 */
void empty_proc(){
	while(1);
}



/**
 * @function get_kernel_map
 * @brief 获取内核
 */
int get_kernel_map(unsigned int *b, unsigned int *l)
{
	Elf32_Ehdr *elf_header = (Elf32_Ehdr*)(boot_params.kernel_file);
	//the kernel file should be in ELF format
	if(memcmp(elf_header->e_ident, ELFMAG, SELFMAG) != 0){
		return -1;
	}
	*b = ~0;
	unsigned int t = 0;
	int i;
	for(i=0;i<elf_header->e_shnum;i++){
		Elf32_Shdr * section_header = (Elf32_Shdr*)(boot_params.kernel_file + elf_header->e_shoff + i*elf_header->e_shentsize);
		if(section_header->sh_flags & SHF_ALLOC){
			int bottom = section_header->sh_addr;
			int top = section_header->sh_addr + section_header->sh_size;
			if(*b>bottom){
				*b = bottom;
			}
			if(t<top){
				t=top;
			}
		}
	}
	assert(*b<t);
	*l = t-*b-1;
	return 0;
}
