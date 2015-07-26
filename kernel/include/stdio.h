#include "type.h"
#include "syscall.h"
#ifndef _DIYOS_STDIO_H
#define _DIYOS_STDIO_H
/**
 * @def MAX_PATh
 * @brief 最大文件路径长度
 */
#define MAX_PATH	128

/**
 * @def O_CREATE & O_RDWT
 * @brief open flags参数取值
 */
#define O_CREATE	1
#define O_RDWT		2


#define STDIN		0
#define STDOUT		1
extern int open(const char * pathname, int flags);
extern int mkdir(const char *pathname);
extern int rmdir(const char *pathname);
extern int close(int fd);
extern int read(int fd, void *buf, int count);
extern int write(int fd, const void * buf, int count);
#define SEEK_START	0
#define SEEK_SET	1
#define SEEK_END	2
extern int seek(int fd, int offset, int where);
extern long tell(int fd);
extern int unlink(const char * pathname);
extern int vsprintf(char *buffer, const char *fmt, va_list args);
extern int sprintf(char *buffer, const char* fmt, ...);
extern int fprintf(int fd, const char *fmt, ...);
extern int printf(const char *fmt, ...);
//stat函数 获取文件信息
/**
 * @struct stat
 * @brief 文件信息
 */
 struct stat {
	int st_dev;			/* major/minor device number */
	int st_ino;			/* i-node number */
	int st_mode;		/* file mode, protection bits, etc. */
	int st_rdev;		/* device ID (if special file) */
	int st_size;		/* file size */
};
/**
 * @function stat
 * @brief get file stat
 * @param path  file pathname
 * @param buf   for output
 * @return 0 success
 */
extern int stat(const char *pathname, struct stat *buf);
#endif
