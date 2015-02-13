/******************************************************/
/*                 操作系统的文件系统                 */
/******************************************************/
#include "type.h"
#include "syscall.h"
#include "global.h"
#include "assert.h"
#include "hd.h"
#include "fs.h"
static void init_fs();
static void mkfs();
/**
 * Main Loop 
 */
void task_fs()
{
	printf("Task FS begins.\n");
/*
	struct message msg;
	msg.type = DEV_OPEN;
	msg.DEVICE = MINOR(ROOT_DEV);		//ROOT_DEV=MAKE_DEV(DEV_HD, MINOR_hd2a) 是文件系统的根分区 我们的次设备号定义 hd2a 就是第2主分区（扩展分区）的第一个逻辑分区，主设备号是DEV_HD 表明设备是硬盘
	//这个设备号的主设备号对应的是消息的目标，根据dd_map可以由主设备号映射到驱动进程的pid，也就是消息的目标
	//次设备号就是msg中的DEVICE
	assert(dd_map[MAJOR(ROOT_DEV)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_pid, &msg);
	printf("close dev\n");
	msg.type=DEV_CLOSE;
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_pid, &msg);
	spin("FS");
*/
	init_fs();
	while(1){}
}

/**
 * @function init_fs
 * @brief 初始化文件系统
 */
void init_fs()
{
	//打开硬盘设备
	struct message msg;
	msg.type = DEV_OPEN;
	msg.DEVICE = MINOR(ROOT_DEV);
	assert(dd_map[MAJOR(ROOT_DEV)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_pid, &msg);
	//打开后创建文件系统
	mkfs();
}

/**
 * @function mkfs
 * @brief Make a avaliable DIYOS's FS in the disk. It will
 *	- Write a super block to sector 1.
 *	- Create three special files:dev_tty0, dev_tty1, dev_tty2
 *	- Create the inode map
 *	- Create the sector map
 *	- Create the inodes of the files
 *	- Create '/', the root directory
 * @return
 */
void mkfs()
{
	struct message msg;
	int i, j;
	int bits_per_sect = SECTOR_SIZE * 8; //8 bits per byte
	//get the geometry of ROOTDEV
	struct part_info geo;
	msg.type	= DEV_IOCTL;
	msg.DEVICE	= MINOR(ROOT_DEV);
	msg.REQUEST	= DIOCTL_GET_GEO;
	msg.BUF		= &geo;
	msg.PID		= TASK_FS;
	assert(dd_map[MAJOR(ROOT_DEV)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_pid, &msg);
	printf("dev size: 0x%x sectors\n", geo.size);
	//super block
	struct super_block sb;
	sb.magic		= MAGIC_V1;
	sb.inodes_count		= bits_per_sect;
	sb.inode_sects_count	= sb.inodes_count * INODE_SIZE/SECTOR_SIZE;
	sb.sects_count		= geo.size;
	sb.imap_sects_count	= 1;
	sb.smap_sects_count	= sb.sects_count/bits_per_sect + 1;
	sb.first_sect		= 1 + 1 /*boot sector & super block*/+ sb.imap_sects_count + sb.smap_sects_count + sb.inode_sects_count;
	sb.root_inode		= ROOT_INODE;
	sb.inode_size		= INODE_SIZE;
	struct inode x;
	sb.inode_isize_off	= (int)&x.i_size - (int)&x;
	sb.inode_start_off	= (int)&x.i_start_sect - (int)&x;
	sb.dir_ent_size		= DIR_ENTRY_SIZE;
	struct dir_entry de;
	sb.dir_ent_inode_off	= (int)&de.inode_idx - (int)&de;
	sb.dir_ent_fname_off	= (int)&de.name - (int)&de;
	memset(fsbuf, 0x90, SECTOR_SIZE);
	memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE);
	//write the super block
	WRITE_SECT(ROOT_DEV, 1);  //sector 0为boot sector
				  //sector 1super block 写入1
	printf("devbase:0x%x00, sb:0x%x00, imap:0x%x00, smap:0x%x00 "
		"inodes:0x%x00 1st_sector:0x%x00\n",	
		geo.base * 2,
		(geo.base + 1) * 2,
		(geo.base + 1 + 1) * 2,
		(geo.base + 1 + 1 + sb.imap_sects_count) * 2,
		(geo.base + 1 + 1 + sb.imap_sects_count + sb.smap_sects_count) * 2,
		(geo.base + sb.first_sect) * 2);

	//inode map
	memset(fsbuf, 0, SECTOR_SIZE);
	for(i=0;i<(CONSOLE_COUNT + 2);i++){
		fsbuf[0] |= 1<<i;
	}
	assert(fsbuf[0] == 0x1F);/* 0001 1111
				  *    | ||||
				  *    | |||`----bit 0: reserved
				  *    | ||`-----bit 1:the first inode, '/'
				  *    | |`------bit 2:/dev_tty0
				  *    | `-------bit 3:/dev_tty1
				  *    `---------bit 4:/dev_tty2
				  */
	WRITE_SECT(ROOT_DEV, 2);//secotr 2为inode sector
	//sector map
	memset(fsbuf, 0, SECTOR_SIZE);
	int nr_sects = DEFAULT_FILE_SECTS_COUNT + 1;
	/*	       ~~~~~~~~~~~~~~~~~~~~~~~|~~~|
	 *                                    |   `---bit 0 is reserved
	 *                                    `-------for '/'
	 */
	for(i=0;i<nr_sects/8;i++){
		fsbuf[i] = 0xFF;
	}
	for(j=0;j<nr_sects % 8;j++){
		fsbuf[i] |= (1<<j);
	}
	WRITE_SECT(ROOT_DEV, 2 + sb.imap_sects_count);
	
	memset(fsbuf, 0, SECTOR_SIZE);
	for(i=1;i<sb.smap_sects_count;i++){
		WRITE_SECT(ROOT_DEV, 2 + sb.imap_sects_count + i);
	}

	//inodes
	//inode for '/'
	memset(fsbuf, 0, SECTOR_SIZE);
	struct inode * pi = (struct inode*)fsbuf;
	pi->i_mode = I_DIRECTORY;
	pi->i_size = DIR_ENTRY_SIZE * 4;//4files  '.',dev_tty0, dev_tty1, dev_tty2
	pi->i_start_sect = sb.first_sect;
	pi->i_sects_count = DEFAULT_FILE_SECTS_COUNT;
	//inode for /dev_tty0~2
	for(i=0;i<CONSOLE_COUNT;i++){
		pi=(struct inode*)(fsbuf + (INODE_SIZE * (i+1)));
		pi->i_mode = I_CHAR_SPECIAL;
		pi->i_size = 0;
		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
		pi->i_sects_count = 0;
	}
	WRITE_SECT(ROOT_DEV, 2+sb.imap_sects_count + sb.smap_sects_count);
	//'/'
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry *pde = (struct dir_entry*)fsbuf;
	pde->inode_idx = 1;
	strcpy(pde->name, ".");
	//dir entries of "/dev_tty0~2
	for(i=0;i<CONSOLE_COUNT;i++){
		pde++;
		pde->inode_idx = i+2;
		sprintf(pde->name, "dev_tty%d", i);
	}
	WRITE_SECT(ROOT_DEV, sb.first_sect);
}

/**
 * @function rw_sector
 * R/W a sector via messaging with the corresponding driver
 *
 * @param io_type	DEV_READ or DEV_WRITE
 * @param dev		device number
 * @param pos		Byte offset from/to where to r/w
 * @param pid		To whom the buffer belongs.
 * @param buf		r/w buffer.
 *
 * @return Zero if success.
 */
int rw_sector(int io_type, int dev, u64 pos, int bytes, int pid, void*buf)
{
	assert(io_type == DEV_READ || io_type == DEV_WRITE);
	struct message msg;
	msg.type	= io_type;
	msg.DEVICE	= MINOR(dev);
	msg.POSITION	= pos;
	msg.BUF		= buf;
	msg.CNT		= bytes;
	msg.PID		= pid;
	assert(dd_map[MAJOR(dev)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(dev)].driver_pid, &msg);
	return 0;
}
