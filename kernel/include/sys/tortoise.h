#ifndef _DIYOS_TORTOISE_H
#define _DIYOS_TORTOISE_H
#include "fs.h"
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
 * @def DEFAULT_FILE_SECTS_COUNT
 * @brief 每个文件默认占用的扇区数量
 * 2048（扇区数）* 512B（每个扇区数据大小） = 1MB(每个文件默认占用磁盘空间1MB）
 */
#define DEFAULT_FILE_SECTS_COUNT	2048
/**
 * @def	INODE MACROs
 * @brief invalid inode 0 表示文件在硬盘上不存在
 *	  the root inode    1 root_inode
 *	  invalid_path     -1 表示文件所在的路径是错误的
 */

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

//tortoise 文件系统
extern struct abstract_file_system tortoise;

extern  void init_tortoise();
#endif
