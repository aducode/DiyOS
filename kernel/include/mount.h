#ifndef _DIYOS_MOUNT_H
#define _DIYOS_MOUNT_H
/**
 * @function mount
 * @brief 挂载文件系统
 * @param source 将要挂上的文件系统，通常是一个设备名
 * @param target 文件系统所要挂在的目标目录
 * @return
 */
extern int mount(const char *source, const char *target);

/**
 * @function unmount
 * @brief 卸载文件系统
 * @param target 文件系统挂载到的目录
 * @return
 */
extern int unmount(const char * target);
#endif
