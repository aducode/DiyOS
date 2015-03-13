#include "type.h"
#include "global.h"
#include "proc.h"
#include "protect.h"
#include "syscall.h"
#include "assert.h"
static void init_mm();
static int alloc_mem(int pid, int memsize);
static int do_fork(struct message *msg);
static void do_exit(struct message *msg);
static void do_wait(struct message *msg);
static int free_mem(int pid);
static void cleanup(struct process *p_proc);
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
		int reply;
		switch(msg.type){
			case FORK:
				msg.RETVAL = do_fork(&msg);
				reply = 1;
				break;
			case EXIT:
				do_exit(&msg);
				reply = 0;//进程都退出了，就不需要给进程发送消息了
				break;
			case WAIT:
				do_wait(&msg);
				reply = 0; //进程会返回到父进程，所以页不需要在给这个进程发消息了
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
	int pid = msg->source;
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
	//int pid = msg->source; //pid是parent pid
	u16  child_ldt_sel = p->ldt_sel;
	while(!proc_table[pid].p_flags); //wait for the process to block
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
 * @function do_exit
 * @brief 进程退出
 *        If proc A calls exit(), then MM will do the following in this routine:
 *		<1> inform FS so that the fd-related things will be cleaned up
 *		<2> free A's memory
 *		<3> set A.exit_status, which is for the parent
 *		<4> depends on parent's status, if parent (say P) is:
 *			(1) WAITING
 *				- clean P's WAITING bit, and
 *				- send P a message to unblock it
 *				- release A's proc_table[] slot
 *			(2) not WAITING
 *				- set A's HANGING bit
 *		<5> iterrate proc_table[], if proc B is found as A's child, then
 *			(1) make INIT the new parent of B, and
 *			(2) if INIT is WAITING and B is HANGING, then:
 *				- clean INIT's WATING bit, and
 *				- send INIT a message to unblock it
 *				- release B's proc_table[] slot
 *			    else
 *			      if INIT is WAITING but B is not HANGING, then
 *				- B will call exit()
 *			      if B is HANGING but INIT is not WAITING, then
 *				- INIT will call wait()
 * TERMs:
 *	- HANGING: everything except the proc_table entry has been cleaned up.
 *	  	   进程已经被清除了，只是在proc_table中还保留一个该进程的slot，只是为了保存进程的exit_status,这个值在将来父进程调用wait()后返回给父进程
 *		   当父进程wait返回后，子进程HANGING状态将取消，并彻底回首proc_table slot，至此exit函数完成，进程关闭完成
 *	- WAITING: a proc has at least one child, and it is waiting for the child to exit()
 *		   如果进程有子进程，并且处于等待子进程exit的状态
 *	- zombie: say P has a child A, A will become a zombie if
 *		- A exit(), and
 *		- P does not wait(), neither does it exit(), that is to sya, P just keeps running without terminating itself or its child
 *
 * @param msg message ptr
 *
 */
void do_exit(struct message *msg)
{
	int status = msg->STATUS;
	int pid = msg->source;
	if(pid==INIT) return;
	struct process *p = &proc_table[pid];
	int parent_pid = p->p_parent;
	int i;
	//tell FS, see fs_exit();
	struct message msg2fs;
	msg2fs.type = EXIT;
	msg2fs.PID = pid;
	send_recv(BOTH, TASK_FS, &msg2fs);
	assert(msg2fs.type == SYSCALL_RET);
		
	free_mem(pid);
	
	p->exit_status = status;
	
	if(proc_table[parent_pid].p_flags & WAITING){//parent is waiting
		proc_table[parent_pid].p_flags &= ~WAITING;
		cleanup(&proc_table[pid]);
	} else {
		proc_table[pid].p_flags |= HANGING;
	}
	//if the proc has any child, make INIT the new parent
	for(i=0;i<TASKS_COUNT + PROCS_COUNT;i++){
		if(proc_table[i].p_parent == pid){
			//
			proc_table[i].p_parent = INIT;
			if((proc_table[INIT].p_flags & WAITING) && (proc_table[i].p_flags & HANGING)){
				proc_table[INIT].p_flags &= ~WAITING;
				cleanup(&proc_table[i]);
			}
		}
	}
}

/**
 * @function do_wait
 * @brief 进程返回到父进程
 *
 * If proc P calls wait(), then MM will do the following in this routine:
 *	<1> iterate proc_table[],
 *		if proc A is found as P's child and it is HANGING
 *			- reply to P (cleanup() will send P a messageto unblock it)
 *			- release A's prooc_table entry
 *			- return (MM will go on with the next message loop)
 *	<2> if no child of P is HANGING
 *		- set P's WAITING bit
 *	<3> if P has no child at all
 *		- reply to P with error
 *	<4> return (MM will go on with the mext message loop)
 *
 * @param msg message ptr
 *
 */
void do_wait(struct message *msg)
{
	int pid = msg->source;
	int i;
	int children = 0;
	struct process *p = proc_table;
	for(i=0;i<TASKS_COUNT + PROCS_COUNT;i++, p++){
		if(p->p_parent == pid){
			//找到pid的子进程了
			children ++;
			if(p->p_flags & HANGING){
				//子进程HANGING
				cleanup(p);
				return; //一次fork出子进程，那么父进程wait只能等待一个子进程的exit
			}
		}
	}
	if(children){
		//has children, but no child is HANGING
		proc_table[pid].p_flags |= WAITING;
	} else {
		//no child at all
		struct message m;
		m.type = SYSCALL_RET;
		m.PID = NO_TASK;
		send_recv(SEND, pid, &m);
	}	
}


/**
 * @functin free_mem
 * @brief Free a memory block. because a memory block is corresponding with a PID, so we don't need to really free anything. In another word, a memory block is dedicated to one and only one PID, no matter what proc actually uses this PID.
 * 现在内存分配方案是根据PID进程线性分配，所以暂时不需要真正的free内存操作，因为PID变成free slot,所以下次会重新使用这块内存的
 * @param pid Whose memory is to be freed.
 *
 * @return Zero if success
 */
int free_mem(int pid)
{
	return 0;
}
/**
 * @function cleanup
 * @brief Do the last jobs to clean up a proc thoroughly:
 *	- Send proc's parent a message to unblock it, and
 *	- releas proc's proc_table entry
 *
 */
void cleanup(struct process *p_proc)
{
	struct message msg2parent;
	msg2parent.type = SYSCALL_RET;
	msg2parent.PID = proc2pid(p_proc);
	msg2parent.STATUS = p_proc->exit_status;
	send_recv(SEND, p_proc->p_parent, &msg2parent);
	p_proc->p_parent = NO_TASK;	
	p_proc->p_flags = FREE_SLOT;
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
