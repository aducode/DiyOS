#include "type.h"
#include "protect.h"
#include "proc.h"
#include "global.h"
#include "assert.h"

static int ldt_seg_linear(struct process *p_proc, int idx);
/**
 * 进程调度
 */
void schedule()
{	
	static int i=0;
	i++;
	if(i>=TASKS_COUNT + PROCS_COUNT) i=0;
	p_proc_ready = proc_table+i;
}
/**
 * 计算线程ldt中idx索引的线性地址
 */
int ldt_seg_linear(struct process *p_proc, int idx)
{
	struct descriptor *p_desc = &(p_proc->ldts[idx]);
//	struct descripotr *p_desc = &p_proc->ldts[idx];//不带括号就会出错
	return p_desc->base_high << 24 | p_desc->base_mid << 16 |p_desc->base_low;
}
/**
 * 虚拟地址转线性地址(ring1 - ring3使用)
 */
void* va2la(int pid, void *va)
{
	struct process *p_proc = &proc_table[pid];
	u32 seg_base = ldt_seg_linear(p_proc, INDEX_LDT_RW);
	u32 la = seg_base + (u32)va;
	
	if(pid < TASKS_COUNT + PROCS_COUNT )
	{
		assert(la==(u32)va);
	}
	return (void*)la;
}
