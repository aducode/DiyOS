#include "type.h"
#include "stat.h"
#include "string.h"
#include "syscall.h"
#include "assert.h"
/**
 * @function mkdir
 * @brief 创建目录
 * @param pathname 目录
 * @return
 */
int mkdir(const char *pathname)
{
	struct message msg;
	reset_msg(&msg);
	msg.type = MKDIR;
	msg.PATHNAME = (void*)pathname;
	msg.NAME_LEN = strlen(pathname);
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.RETVAL;
}

/**
 * @function rmdir
 * @brief 删除空目录
 * @param pathname 目录
 * @return 
 */
int rmdir(const char *pathname)
{
	struct message msg;
	reset_msg(&msg);
	msg.type = RMDIR;
	msg.PATHNAME = (void*) pathname;
	msg.NAME_LEN = strlen(pathname);
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.RETVAL;
}

/**
 * @function stat
 * @brief get file stat
 * @param pathname  file path
 * @param buf   for output
 * @return 0 success
 */
int stat(const char *pathname, struct stat *buf)
{
	struct message msg;
	reset_msg(&msg);
	msg.type = STAT;
	msg.PATHNAME = (void*)pathname;
	msg.NAME_LEN = strlen(pathname);
	msg.BUF = (void*)buf;
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.RETVAL;
}