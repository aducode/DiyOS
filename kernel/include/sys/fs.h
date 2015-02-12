#ifndef _DIYOS_FS_H
#define _DIYOS_FS_H
/**
 *@struct dev_drv_map fs.h
 * 定义设备和驱动程序（pid）的映射关系
 */
struct dev_drv_map
{
	int driver_pid;	//pid of driver task
};
#endif
