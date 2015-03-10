#include "type.h"
#include "global.h"
#include "proc.h"
#include "protect.h"
#include "syscall.h"
#include "assert.h"
static void init_mm();
static int alloc_mem(int pid, int memsize);
static int do_fork(struct message *msg);
/**
 * @function task_mm
 * @brief Main Loop for Memory Task
 */
void task_mm()
{
	init_mm();
	struct message msg;
	while(1){
		send_recv(RECEIVE, ANY, &msg);
		int src = msg.source;
		int reply = 1;
		switch(msg.type){
			case FORK:
				msg.RETVAL = do_fork(&msg);
				break;
			default:
				dump_msg("MM::unknown msg", &msg);
				assert(0);
				break;
		}
		if(reply){
			msg.type = SYSCALL_RET;
			send_recv(SEND, src, &msg);
		}
	}
}


/**
 * @function init_mm
 * @brief 初始化内存
 */
void init_mm()
{
//	printk("{MM} memsize:%dMB\n", boot_params.mem_size/(1024*1024));
	
}

/**
 * @function do_fork
 * @brief fork
 */
int do_fork(struct message *msg)
{
	//find a free slot in proc_table
	struct process *p = proc_table;
	int i;
	for(i=0;i<TASKS_COUNT + PROCS_COUNT;i++, p++){
		if(p->p_flags == FREE_SLOT){
			break;
		}
	}
	int child_pid = i;
	assert(p==&proc_table[child_pid]);
	assert(child_pid>=TASKS_COUNT + NATIVE_PROCS_COUNT);
	if(i==TASKS_COUNT + PROCS_COUNT){//no free slot
		return -1;
	}
	assert(i<TASKS_COUNT + PROCS_COUNT);
	//duplicate the process table
	int pid = msg->source; //pid是parent pid
	u16  child_ldt_sel = p->ldt_sel;
	*p = proc_table[pid]; //这里是*p=，不是p=,所以相当于将proc_table[pid]结构体拷贝一份到p指针地址
	p->ldt_sel = child_ldt_sel; //将子进程ldt selector还原
	p->p_parent = pid; //设置子进程的父进程
	sprintf(p->name, "%s_%d", proc_table[pid].name, child_pid); //设置子进程名称
	
	//duplicate the process:T, D&S
	struct descriptor *ppd;
	//text segment
	ppd = &proc_table[pid].ldts[0];
	//base of Text segment, in bytes
	int caller_text_base = reassembly(ppd->base_high, 24, ppd->base_mid, 16, ppd->base_low);
	//limit of text segment, in 1 or 4096  bytes
	int caller_text_limit = reassembly( (ppd->limit_high_attr2 & 0xF), 16, 0, 0, ppd->limit_low);
	//size of text segment ,in bytes
	int caller_text_size = ((caller_text_limit + 1)*((ppd->limit_high_attr2 & (DA_LIMIT_4K>>8))?4096:1));
	//data & stack segments
	ppd = &proc_table[pid].ldts[1];
	//base of Data & Stack segment, in  bytes
	int caller_data_stack_base = reassembly(ppd->base_high, 24, ppd->base_mid, 16, ppd->base_low);
	//limit of Data & Stack segment, in 1 or 4096  bytes
	int caller_data_stack_limit = reassembly((ppd->limit_high_attr2 & 0xF), 16, 0, 0, ppd->limit_low);
	//size of Data & Stack segment, in bytes
	int caller_data_stack_size = ((caller_data_stack_limit + 1) * ((ppd->limit_high_attr2 & (DA_LIMIT_4K>>8))?4096:1));
	//we don't separate T, D&S segments, so we have:
	assert((caller_text_base == caller_data_stack_base) &&
	       (caller_text_limit == caller_data_stack_limit) &&
               (caller_text_size == caller_data_stack_size));
	//base of child proc, T, D&S segments share the same space,  so we allocate memory just once
	int child_base = alloc_mem(child_pid, caller_text_size);
	//child is a copy of the parent
	memcpy((void*)child_base, (void*)caller_text_base, caller_text_size);

	//child's LDT
	init_desc(&p->ldts[0],child_base, (PROC_IMAGE_SIZE_DEFAULT - 1)>>LIMIT_4K_SHIFT, DA_LIMIT_4K|DA_32|DA_C|PRIVILEGE_USER<<5);
	init_desc(&p->ldts[1],child_base, (PROC_IMAGE_SIZE_DEFAULT - 1)>>LIMIT_4K_SHIFT, DA_LIMIT_4K|DA_32|DA_DRW|PRIVILEGE_USER<<5);
	
	//tell FS, see fs_fork();
	struct message msg2fs;
	msg2fs.type = FORK;
	msg2fs.PID = child_pid;
	send_recv(BOTH, TASK_FS, &msg2fs);
	//child PID will be returned to the parent proc
	msg->PID = child_pid;
	//birth of the child
	struct message m;
	m.type = SYSCALL_RET;
	m.RETVAL = 0;
	m.PID = 0;
	send_recv(SEND, child_pid, &m);
	return 0;
}



/**
 * @function alloc_mem
 * @brief Allocate a memory block for a proc,
 *        这样的内存分配方案是：为每个进程分配1MB内存，根据pid直接线性映射
 * @param pid Which proc the memory is for
 * @param memsize How many bytes is needed.
 *
 * @return The base of the memory just allocated.
 */
int alloc_mem(int pid, int memsize)
{
	assert(pid>=(TASKS_COUNT + NATIVE_PROCS_COUNT));
	if(memsize > PROC_IMAGE_SIZE_DEFAULT){
		panic("unsupported memory request:%d. (should be less than %d)", memsize, PROC_IMAGE_SIZE_DEFAULT);
	}
	int base = PROCS_BASE + (pid - (TASKS_COUNT + NATIVE_PROCS_COUNT)) * PROC_IMAGE_SIZE_DEFAULT;
	if(base + memsize >= boot_params.mem_size){
		panic("memory allocation faild. pid:%d", pid);
	}
	return base;
}
