#include "tortoise.h"
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
	u32	i_start_sect;	//1.Normal The first sector of the data 起始扇区
				//2.Dev	   The dev number for special file like tty
				//floppy下，表示起始簇号
	u32	i_sects_count;	//How man sectors the file occupies 文件所占扇区数
				//floppy下，无用
	u8	_unused[16];	//对齐 相当于4个u32
	//the following items are only present in memory
	int i_dev;
	int i_cnt;	//how many procs share this inode
	int i_num;	//inode nr
	struct inode * i_parent; //父目录文件的inode
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
 * @def	INODE MACROs
 * @brief invalid inode 0 表示文件在硬盘上不存在
 *	  the root inode    1 root_inode
 *	  invalid_path     -1 表示文件所在的路径是错误的
 */
#define INVALID_PATH	-1
#define INVALID_INODE	0
#define ROOT_INODE		1


/**
 * @def MAX_FILENAME_LEN
 * @brief Max len of a filename
 * @see dir_entry
 */
#define MAX_FILENAME_LEN	12


//functions
/**
 * @function rw_sector
 * R/W a sector via messaging with the corresponding driver
 *
 * @param io_type       DEV_READ or DEV_WRITE
 * @param dev           device number
 * @param pos           Byte offset from/to where to r/w
 * @param size          How many bytes
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
	int 		fd_cnt;		//How many procs share this desc
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

/**
 * @define INSTALL_START_SECT INSTALL_SECTS_COUNT
 * @brief Some sector are reserved for us (the gods of the os) to copy a tar file
 *	there, which will be extracted and used by the os.
 */
//#define INSTALL_START_SECT	0x8000
//#define INSTALL_SECTS_COUNT	0x800


#define root_inode() tortoise.get_inode(0, ROOT_INODE)

//挂载点
//#include "map.h"


////////////////////////  文件系统层通用函数  ////////////////////////////////////
//解耦inode与持久化数据，便于不同类型文件系统分层

//初始化
typedef void (*init_fs_func)(int dev);
//获取inode
typedef struct inode * (*get_inode_func)(struct inode *parent, int inode_idx);
//同步inode到磁盘
typedef void (* sync_inode_func)(struct inode *pinode);
//从目录中获取inode number
typedef int (* get_inode_num_from_dir_func)(struct inode *parent,  const char * filename);
//读写
typedef int (* rdwt_func)(int io_type, struct inode *pinode, void * buf, int pos,  int len);
//创建文件
typedef struct inode * (* create_file_func)(struct inode *parent, const char * filename, int flags);

typedef struct inode * (* create_directory_func)(struct inode *parent, const char * filename, int flags);
//用于创建设备文件
typedef struct inode * (* create_special_file_func)(struct inode *parent, const char * filename, int type, int flags, int dev);
//删除文件
typedef int (* unlink_file_func)(struct inode *pinode);
//判断目录是否为空
typedef int (* is_dir_emtpy_func)(struct inode *pinode);
//新建目录项
typedef void (* new_dir_entry_func)(struct inode *dir_inode, int inode, const char *filename);
//删除目录项
typedef void (* rm_dir_entry_func)(struct inode *dir_inode, int inode);
//mount
typedef void (* mount_func)(struct inode *pinode, int dev);
//卸载
typedef void (* unmount_func)(struct inode *pinode, int newdev);
//磁盘格式化
typedef void (* format_func)();

/**
 * @struct abstract_file_system
 * @brief 文件系统抽象层
 */
struct abstract_file_system{
	int							dev;					//设备号
	int							is_root;				//是否是根设备1是 0 否
	init_fs_func				init_fs;				//初始化文件系统
	get_inode_func				get_inode;				//从持久化数据获取inode
	//sync_inode_func 			sync_inode;				//同步inode
	get_inode_num_from_dir_func	get_inode_num_from_dir;	//从目录中获取inode number
	rdwt_func					rdwt;					//读写
	create_file_func			create_file;			//创建文件
	create_special_file_func	create_special_file;	//创建设备文件
	create_directory_func		create_directory;			//创建目录
	unlink_file_func			unlink_file;			//删除文件
	is_dir_emtpy_func			is_dir_empty;			//是否是空目录
	new_dir_entry_func			new_dir_entry;		//新建目录项
	rm_dir_entry_func			rm_dir_entry;			//删除目录项
	mount_func					mount;					//mount
	unmount_func				unmount;				//unmount
	format_func					format;					//格式化
};
#endif
