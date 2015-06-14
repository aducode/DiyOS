#include "type.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "assert.h"
int printf(const char *fmt, ...)
{
	int i;
	char buf[512];
	va_list arg = (va_list)((char*)(&fmt)+4);
        i = vsprintf(buf,fmt, arg);
        int c = write(1, buf, i);  //调用之前必须保证进程打开stdout
        assert(c==i);
        return i;

	
}
/**
 *printf
 */
int fprintf(int fd, const char *fmt, ...)
{
	int i;
	char buf[1024];
	//开始写成下面的，导致不定参赛出错
	//va_list arg = (va_list)((char*)(&fmt+4));
	//正确写法如下，&fmt取地址（地址类型是4Byte 32bit的，我机器是32位），将4Byte的地址转换成char* 变为1Byte 这样后面+4 就是真正的加4Byte，变成下一个参数首地址
	va_list arg = (va_list)((char*)(&fmt)+4);
	//或者下面这种写法
	//va_list arg = (va_list)((char*)(&fmt+1));
	i = vsprintf(buf,fmt, arg);
	int c = write(fd, buf, i);
	assert(c==i);
	return i;
}
/**
 *格式化输出到字符串
 */
int sprintf(char *buffer, const char *fmt, ...)
{
	int i;
	va_list arg = (va_list)((char*)(&fmt)+4);
	i = vsprintf(buffer, fmt, arg);
	return i;
}
int vsprintf(char *buf, const char * fmt, va_list args)
{
	char *p;
	char tmp[256];
	va_list p_next_arg = args;
	for(p=buf;*fmt;fmt++){
		if(*fmt!='%'){
			*p++ = *fmt;
			continue;	
		}
		fmt++;
		switch(*fmt){
			case 'x':
				itoa(*((int*)p_next_arg),tmp, 16);
				strcpy(p,tmp);	
				p_next_arg += 4;
				p+=strlen(tmp);
				break;
			case 'd':
				itoa(*((int*)p_next_arg),tmp,10);
				strcpy(p, tmp);
				p_next_arg += 4;
				p+=strlen(tmp);
				break;
			case 'o':
				itoa(*((int*)p_next_arg),tmp, 8);
				strcpy(p, tmp);
				p_next_arg += 4;
				p+=strlen(tmp);
				break;
			case 'b':
				itoa(*((int*)p_next_arg), tmp, 2);
				strcpy(p, tmp);
				p_next_arg += 4;
				p+=strlen(tmp);
			case 'c':
				*p++ = *((char*)p_next_arg);
				p_next_arg += 4;
				break;
			case 's':
				//p_next_arg 是char *类型的地址，所以这里要把类型转成char **  代表是char *的地址，然后在*运算符取地址，变成char*类型
				strcpy(p,(*((char**)p_next_arg)));
				p += strlen(*((char**)p_next_arg));
				p_next_arg += 4;
				break;
			default:
				break;
		}
	}
	*p=0;
	return (p-buf);
}

/**
 * @function open
 * @brief 打开文件
 * @param pathname 文件名
 * @param flags 读写标志
 */
int open(const char *pathname, int flags)   //0x1b7c
{
	struct message msg;
	msg.type = OPEN;
	msg.PATHNAME = (void*)pathname;
	msg.FLAGS = flags;
	msg.NAME_LEN = strlen(pathname);
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.FD;
}

/**
 * @function close
 * @brief 关闭文件
 * 
 * @param fd 要关闭的文件句柄
 *
 * @return zero if successful, otherwise -1.
 */
int close(int fd)
{
	struct message msg;
	msg.type	= CLOSE;
	msg.FD		= fd;
	send_recv(BOTH, TASK_FS, &msg);
	return msg.RETVAL;
}


/**
 * @function read
 * @brief 读文件
 * 
 * @param fd 文件句柄
 * @param buf 缓冲
 * @param count
 *
 * @return 读的byte数
 */
int read(int fd, void *buf, int count)
{
	struct message msg;
	msg.type = READ;
	msg.FD = fd;
	msg.BUF = buf;
	msg.CNT = count;
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type==SYSCALL_RET);
	return msg.CNT;
}

/**
 * @function write
 * @brief  写文件
 * 
 * @param fd 文件句柄
 * @param buf 数据
 * @param count
 *
 * @return  写入的字节数
 */
int write(int fd, const void * buf, int count)
{
	struct message msg;
	msg.type = WRITE;
	msg.FD = fd;
	msg.BUF = (void*)buf;
	msg.CNT = count;
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.CNT;
}

/**
 * @function seek
 * @brief 设置文件内部指针位置
 * @param fd
 * @param offset 偏移
 * @param where 起始位置SEEK_SET:相对于当前位置 SEEK_START: 相对于开始位置 SEEK_END:相对于结束位置
 * @return 0 成功
 */
int seek(int fd, int offset, int where)
{
	struct message msg;
	msg.type = SEEK;
	msg.FD = fd;
	msg.OFFSET = offset;
	msg.WHERE = where;
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.RETVAL;
}

/**
 * @function tell
 * @brief 文件指针当前位置
 * @param fd
 * @return 位置
 */
long tell(int fd)
{
	struct message msg;
	msg.type = TELL;
	msg.FD = fd;
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);	
	return msg.POSITION;
}

/**
 * @function unlink
 * @brief 删除文件
 * @param pathanem  文件名
 * 
 * @return 0 successful -1 error
 */
int unlink(const char * pathname)
{
	struct message msg;
	msg.type = UNLINK;
	msg.PATHNAME = (void*)pathname;
	msg.NAME_LEN = strlen(pathname);
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.RETVAL;
}
