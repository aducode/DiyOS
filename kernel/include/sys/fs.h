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
/**
 * @def MAGIC_V1
 * @brief Magic number of FS v1.0
 */
#define MAGIC_V1	0x111 
/**
 * @struct super_block fs.h
 * @brief The 2nd sector of the FS
 * 文件系统是在硬盘分区之上的由操作系统定义的持久化数据组织格式
 * super_block相当于我们文件系统的目录
 * 超级块在分区的第一个扇区上(512B)
 * Remember to change SUPER_BOLCK_SIZE if the members are changed
 */
struct super_block{
	u32	magic;		//文件系统的魔数
	u32	inodes_count;	//inode count
	u32	sects_count;	//sector count 
	u32 	imap_sects_count;//inode-map sectors count
	u32	smap_sects_count;//sector-map sectors count
	u32	first_sect;	//Number of the first data sector
	u32	inode_sects_count;//inode sectors count
	u32	root_inode;	//inode number of root directory
	u32	inode_size;	//INODE_SIZE
	u32	inode_isize_off;//offset of struct inode::i_size
	u32	inode_start_off;//offset of struct inode::i_start_sect
	u32	dir_ent_size;	//DIR_ENTRY_SIZE
	u32	dir_ent_inode_off;//Offset of struct dir_entry::inode_idx
	u32	dir_ent_fname_off;//Offset of struct dir_entry::name
	//the following item(s) are only present in memory
	
	int sb_dev;		//the super block's home device
};


/**
 * @def SUPER_BLOCK_SIZE
 * @brief The size of super block \b in \b the \b device
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 * 就是上面的结构体持久化到硬盘所用的大小（sb_dev不需要持久化），上面结构体除去sb_dev外共有14个u32类型成员，所以14*4byte = 56byte
 */
#define SUPER_BLOCK_SIZE	56
/**
 * @def MAX_SUPER_BLOCK_COUNT
 * @brief 
 */
#define MAX_SUPER_BLOCK_COUNT	8

/**
 * @struct inode
 * @brief i-node
 *
 * The \c start_sect and\c nr_sects locate the file in the device,
 * and the size show how many bytes is used
 * If <tt> size<(sects_count * SECTOR_SIZE)</tt>, the rest bytes are wasted and reserved for later writing.
 *
 * \bNote: Remember to change INODE_SIZE if the members are changed
 */
struct inode {
	u32	i_mode;		//Access mode
	u32	i_size;		//File size	文件大小
	u32	i_start_sect;	//The first sector of the data 起始扇区
	u32	i_sects_count;	//How man sectors the file occupies 文件所占扇区数
	u8	_unused[16];	//对齐 相当于4个u32
	//the following items are only present in memory
	int i_dev;
	int i_cnt;	//how many procs share this inode
	int i_num;	//inode nr
};
/**
 * @def i_mode types
 * @brief 定义不同类型的文件
 */
/* INODE::i_mode (octal, lower 12 bits reserved) */
#define I_TYPE_MASK     0170000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY     0040000
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE	0010000

/**
 * @def is_special
 * @brief 判断是否是特殊文件入块设备文件或tty文件
 */
#define is_special(m) ((((m) & TYPE_MASK) == I_BLOCK_SPECIAL) || (((m) & I_TYPE_MASK) == I_CHAR_SPECIAL))
/**
 * @def DEFAULT_FILE_SECTS_COUNT
 * @brief 每个文件默认占用的扇区数量
 * 2048（扇区数）* 512B（每个扇区数据大小） = 1MB(每个文件默认占用磁盘空间1MB）
 */
#define DEFAULT_FILE_SECTS_COUNT	2048
/**
 * @def	INODE MACROs
 * @brief invalid inode
 *	  the root inode
 */
#define INVALID_INODE	0
#define ROOT_INODE	1
/**
 * @def INODE_SIZE
 * @brief The size of i-node stored \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device,\b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define INODE_SIZE	32

/**
 * @def MAX_INODE_COUNT
 * @brief 系统支持的的最大inode数量
 */
#define MAX_INODE_COUNT	64

/**
 * @def MAX_FILENAME_LEN
 * @brief Max len of a filename
 * @see dir_entry
 */
#define MAX_FILENAME_LEN	12

/**
 * @struct dir_entry
 * @brief Directory Entry
 */
struct dir_entry{
	int inode_idx;	//inode_idx
	char name[MAX_FILENAME_LEN];	//file name
};

/**
 * @def DIR_ENTRY_SIZE
 * @brief The size of directory entry in the device.
 *
 * It is as same as the size in memory
 */
#define DIR_ENTRY_SIZE sizeof(struct dir_entry)


//functions
/**
 * @function rw_sector
 * R/W a sector via messaging with the corresponding driver
 *
 * @param io_type       DEV_READ or DEV_WRITE
 * @param dev           device number
 * @param pos           Byte offset from/to where to r/w
 * @param size		How many bytes
 * @param pid           To whom the buffer belongs.
 * @param buf           r/w buffer.
 *
 * @return Zero if success.
 */
extern int rw_sector(int io_type, int dev, u64 pos, int bytes, int pid, void*buf);

/**
 * @def READ_SECT
 * @brief 对rw_sector函数封装一下 读一个扇区的数据
 */
#define READ_SECT(dev, sect_nr) rw_sector(DEV_READ,\
					dev,	\
					(sect_nr) * SECTOR_SIZE,\
					SECTOR_SIZE,\
					TASK_FS,\
					fsbuf);
/**
 * @def WRITE_SECT
 * @brief 对rw_sector函数封装一下 写入数据到一个扇区
 */
#define WRITE_SECT(dev, sect_nr) rw_sector(DEV_WRITE,\
					dev,	\
					(sect_nr) * SECTOR_SIZE,\
					SECTOR_SIZE,\
					TASK_FS,\
					fsbuf);
/**
 * @struct  file_desc
 * @brief 文件描述符
 * 系统文件描述符表作为全局变量统一维护，每个进程中保存着一个文件描述符表的指针数组，
 * process中文件的fd就是进程中文件描述符指针表的下标，fd找到file_desc指针，这个指针指向全局文件描述符表的某一项。
 * 文件描述符中保存着inode指针，系统全局中有一个inode table，这个指针就指向inode table的某一项
 */
struct file_desc {
	int		fd_mode;	//R or W
	int 		fd_pos;		//Current position for R/W
	struct inode *	fd_inode;	//Ptr to the i-node
};


/**
 * @def MAX_FILE_DESC_COUNT
 * @brief 最大文件描述符数量
 */
#define MAX_FILE_DESC_COUNT	64


/**
 * @def MAX_FILE_COUNT
 * @brief 最大文件数
 * @see proc.h
 * 这个定义在proc.h里，因为struct process里面会保存一个struct file_desc filp[]
 */
//#define MAX_FILE_COUNT	64
#endif
