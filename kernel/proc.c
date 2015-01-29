#include "proc.h"
#include "global.h"
/**
 * 进程调度
 */
void schedule()
{	
	static int i=0;
	i++;
	if(i>=MAX_PROCESS_COUNT) i=0;
	p_proc_ready = proc_table+i;
}

