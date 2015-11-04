#include "type.h"
#include "syscall.h"
#include "assert.h"

/**
 * @function mount
 * @brief 挂载文件系统
 * @param source 将要挂上的文件系统，通常是一个设备名
 * @param target 文件系统所要挂在的目标目录
 * @return
 */
int mount(const char *source, const char *target)
{
	struct message msg;
	reset_msg(&msg);
	msg.type = MOUNT;
	msg.PATHNAME = (void*)target;
	msg.NAME_LEN = strlen(target);
	msg.DEVNAME = (void*)source;
	msg.DEVNAME_LEN = strlen(source);
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.RETVAL;
}

/**
 * @function unmount
 * @brief 卸载文件系统
 * @param target 文件系统挂载到的目录
 * @return
 */
int unmount(const char * target)
{
	struct message msg;
	reset_msg(&msg);
	msg.type = UNMOUNT;
	msg.PATHNAME = (void*)target;
	msg.NAME_LEN = strlen(target);
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.RETVAL;
	return -1;
}
