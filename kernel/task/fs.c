/******************************************************/
/*                 操作系统的文件系统                 */
/******************************************************/
#include "type.h"
#include "syscall.h"
#include "assert.h"
/**
 * Main Loop 
 */
void task_fs()
{
	printf("Task FS begins.\n");
	struct message msg;
	msg.type = DEV_OPEN;
	send_recv(BOTH, TASK_HD, &msg);
	
	spin("FS");
}
