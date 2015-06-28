#include "type.h"
#ifndef _DIYOS_FLOPPY_H
#define _DIYOS_FLOPPY_H
/**
 * @function init_floppy
 * @brief 初始化floppy 中断
 * @return
 */
extern void init_floppy();

/**
 * @function floppy_open
 * @brief 打开设备
 * @param device 设备号
 * @return
 */
extern void floppy_open(int device);

/**
 * @function floppy_close
 * @brief 关闭设备
 * @param device 设备号
 * @return
 */
extern void floppy_close(int device);

/**
 * @function floppy_rdwt
 * @brief 设备读写数据
 * @param msg 消息
 * @return
 */
extern void floppy_rdwt(struct message *msg);

/**
 * @function floppy_ioctl
 * @brief 控制设备
 * @param msg 消息
 */
extern void floppy_ioctl(struct message *msg);
#endif
