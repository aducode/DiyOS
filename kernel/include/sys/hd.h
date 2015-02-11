#ifndef _DIYOS_HD_H
#define _DIYOS_HD_H
#define MAK_DEVICE_REG(lba, drv, lba_highest) (((lba)<<6) | \
					       ((drv)<<4) | \
					       (lba_highest & 0xF)|0xA0)
extern void init_hd();
#endif
