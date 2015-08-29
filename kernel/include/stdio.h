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
/**
 * @function open
 * @brief 打开文件
 * @param pathname 文件名
 * @param flags 读写标志
 */
extern int open(const char * pathname, int flags);

/**
 * @function close
 * @brief 关闭文件
 * 
 * @param fd 要关闭的文件句柄
 *
 * @return zero if successful, otherwise -1.
 */
extern int close(int fd);

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
extern int read(int fd, void *buf, int count);

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
extern int write(int fd, const void * buf, int count);

#define SEEK_START	0
#define SEEK_SET	1
#define SEEK_END	2

/**
 * @function seek
 * @brief 设置文件内部指针位置
 * @param fd
 * @param offset 偏移
 * @param where 起始位置SEEK_SET:相对于当前位置 SEEK_START: 相对于开始位置 SEEK_END:相对于结束位置
 * @return 0 成功
 */
extern int seek(int fd, int offset, int where);

/**
 * @function tell
 * @brief 文件指针当前位置
 * @param fd
 * @return 位置
 */
extern long tell(int fd);

/**
 * @function unlink
 * @brief 删除文件
 * @param pathanem  文件名
 * 
 * @return 0 successful -1 error
 */
extern int unlink(const char * pathname);
extern int vsprintf(char *buffer, const char *fmt, va_list args);
extern int sprintf(char *buffer, const char* fmt, ...);

/**
 * @function fprintf
 * @brief 格式化输出到文件
 * @param fd 输出的fd
 * @param fmt 格式
 * @param ... 格式数据
 *
 * @return
 */
extern int fprintf(int fd, const char *fmt, ...);

/**
 * @function printf
 * @brief 格式化输出到stdout
 * @param fmt 格式
 * @param ... 格式化数据
 *
 * @return
 */
extern int printf(const char *fmt, ...);

/**
 * @function rename
 * @brief 重命名文件，可以用来实现移动文件功能
 * @param oldname 旧文件名
 * @param newname 新文件名
 * @return 0 if success
 */
extern int rename(const char *oldname, struct char *newname);
#endif
