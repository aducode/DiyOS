#include "type.h"
#ifndef _DIYOS_STAT_H
#define _DIYOS_STAT_H
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
#ifdef _WITH_TEST_
	int i_cnt;		/*引用计数*/
#endif
};
/**
 * @function stat
 * @brief get file stat
 * @param path  file pathname
 * @param buf   for output
 * @return 0 success
 */
extern int stat(const char *pathname, struct stat *buf);

/**
 * @function rmdir
 * @brief 删除空目录
 * @param pathname 目录名
 *
 * @return 0 if successful
 */
extern int rmdir(const char *pathname);

/**
 * @function mkdir
 * @brief 创建目录
 * @param pathname 目录
 * @return 0 if Successful
 */
extern int mkdir(const char *pathname);

/**
 * @function chdir
 * @brief 修改进程所在目录
 * @param pathname 目录名
 * @return 0 if success
 */
extern int chdir(const char *pathname);
#endif
