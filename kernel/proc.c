#include "proc.h"
#include "global.h"
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

