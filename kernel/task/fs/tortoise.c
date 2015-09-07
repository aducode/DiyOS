/******************************************************/
/*                   tortoise文件系统                 */
/*      tortoise(乌龟)是我自己的文件系统的名字        */
/* 之所以叫tortoise是因为我们的文件系统读写慢/扩展难  */
/******************************************************/
//主要供fs.c使用，用来将fat12格式的文件系统，适配到自己的文件系统.
#include "fs.h"
#include "hd.h"
#include "syscall.h"
#include "assert.h"
#include "tortoise.h"

/**
 * @function init_tortoise
 * @brief 初始化文件系统
 */
static void init_tortoise_fs(int dev);

//MAX_SUPER_BLOCK_COUNT目前是8
//每个设备都有一个超级块
static struct super_block super_block[MAX_SUPER_BLOCK_COUNT];

static void fmtfs();
static void init_super_block(struct super_block * p_sb);
static void init_inode_bitmap();
static void init_sect_bitmap(struct super_block * p_sb);
static void init_inode_array(struct super_block * p_sb);
static void init_data_blocks(struct super_block * p_sb);

static void read_super_block(int dev);
static struct super_block * get_super_block(int dev);

static int alloc_imap_bit(int dev);
static int alloc_smap_bit(int dev, int sects_count_to_alloc);
static struct inode* new_inode(struct inode *parent, int inode_nr, u32 i_mode, u32 start_sect,u32 i_sects_count, u32 i_size);
static void sync_inode(struct inode *p);

static struct inode * create_file(const char *filename, struct inode *dir_inode , int flags);
static struct inode * create_special_file(const char *filename, struct inode *dir_inode , int flags, int dev);

// 新建dir entry 
static void new_dir_entry(struct inode *dir_inode, int inode_nr, const char *filename);
// 删除 dir entry
static void rm_dir_entry(struct inode *dir_inode, int inode_nr);

struct abstract_file_system tortoise = {
	ROOT_DEV,						//dev
	1,								//is_root
	init_tortoise_fs,				//init_fs_func
	get_inode_fat12,				//get_inode_func
	//sync_inode_fat12,				//sync_inode_func
	get_inode_idx_from_dir_fat12,	//get_inode_num_from_dir_func
	do_rdwt_fat12,					//rdwt_func
	create_file,					//create_file_func
	create_special_file,			//create_special_file_func
	unlink_file_fat12,				//unlink_file_func
	is_dir_emtpy,					//is_dir_emtpy_func
	//new_dir_entry_fat12,			//new_dir_entry_func
	//rm_dir_entry_fat12,				//rm_dir_entry_func
	mount_for_fat12,				//mount_func
	fmtfs							//format_func
};

void init_tortoise() {
	//super_block[]
	struct super_block *sb = super_block;
	for(;sb<&super_block[MAX_SUPER_BLOCK_COUNT];sb++){
		sb->sb_dev = NO_DEV;
	}
}
void init_tortoise_fs(int dev)
{
	//open the device:hard disk
	//打开硬盘设备
	//设定硬盘为根文件系统，所以这里给HD发送DEV_OPEN消息
	//对于其他的存储介质，统一抽象成/dev/xxx
	//比如软盘 /dev/floppy
	//将来对软盘的操作由mount unmount来完成
	struct message msg;
	msg.type = DEV_OPEN;
	msg.DEVICE = MINOR(ROOT_DEV);
	assert(dd_map[MAJOR(ROOT_DEV)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_pid, &msg);
	//打开后创建文件系统
	fmtfs();
	//load super block of ROOT
	read_super_block(ROOT_DEV);
	sb=get_super_block(ROOT_DEV);
	assert(sb->magic == MAGIC_V1);
}




/**
 * @function fmtfs
 * @brief 格式化文件系统
 *	- 写入超级块
 *	- 创建'\'根目录
 * @return
 */
void fmtfs()
{
	struct super_block sb;
	//step 1 初始化超级块
	init_super_block(&sb);
	//step 2 初始化inode bitmap
	init_inode_bitmap();
	//step 3 初始化sector bitmap
	init_sect_bitmap(&sb);
	//step 4 初始化inode array
	init_inode_array(&sb);
	//step 5 初始化数据块
	init_data_blocks(&sb);	
}

/**
 * @function init_super_block
 * @brief 初始化super block，并写入磁盘
 * @param p_sb outparam
 * @return
 */
void init_super_block(struct super_block* p_sb)
{
	//step 1 获取硬盘分区信息
	struct message msg;
	int bits_per_sect = SECTOR_SIZE * 8;
	struct part_info geo;
	msg.type		= DEV_IOCTL;
	msg.DEVICE		= MINOR(ROOT_DEV);
	msg.REQUEST		= DIOCTL_GET_GEO;
	msg.BUF			= &geo;
	msg.PID			= TASK_FS;
	assert(dd_map[MAJOR(ROOT_DEV)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_pid, &msg);
	//step 2 写入超级块
	p_sb->magic		= MAGIC_V1;
	p_sb->inodes_count	= bits_per_sect;
	//inode array所占的扇区数
	p_sb->inode_sects_count	= p_sb->inodes_count * INODE_SIZE/SECTOR_SIZE;
	//扇区总数
	p_sb->sects_count	= geo.size;
	//inode bitmap 所占扇区数1个扇区
	p_sb->imap_sects_count	= 1;
	//sector map所占扇区数 多预留一位
	p_sb->smap_sects_count	= p_sb->sects_count/bits_per_sect + 1;
	p_sb->first_sect	= 1 + 1 + p_sb->imap_sects_count + p_sb->smap_sects_count + p_sb->inode_sects_count;//数据区的第一个扇区，跳过boot sector和super block sector和inode bitmap 和 sector bitmap 和inode array扇区
	p_sb->root_inode	= ROOT_INODE;	//inode bitmap 下标1
	p_sb->inode_size	= INODE_SIZE;	//每个inode结构体持久化大小
	struct inode x;
	p_sb->inode_isize_off	= (int)&x.i_size - (int)&x;
	p_sb->inode_start_off	= (int)&x.i_start_sect - (int)&x;
	p_sb->dir_ent_size	= DIR_ENTRY_SIZE;
	struct dir_entry de;
	p_sb->dir_ent_inode_off	= (int)&de.inode_idx - (int)&de;
	p_sb->dir_ent_fname_off	= (int)&de.name - (int)&de;
	memset(fsbuf, 0x90, SECTOR_SIZE);
	memcpy(fsbuf, (void*)p_sb, SUPER_BLOCK_SIZE);
	//super block写入磁盘
	WRITE_SECT(ROOT_DEV, 1);//写入扇区1（0为boot扇区，不使用)
	
}

/**
 * @function init_inode_bitmap
 * @brief 初始化inode bitmap扇区
 * @return
 */
void init_inode_bitmap()
{
	int i;
	memset(fsbuf, 0, SECTOR_SIZE);
	for(i=0;i<2;i++){
		fsbuf[0] |= 1<<i;
	}
	assert(fsbuf[0] == 0x03); /* 0000 0011
				   *        ||
				   *        |`bit 0: reserved
				   *        `-bit 1: the first inode /
                                   */
	WRITE_SECT(ROOT_DEV, 2); //sector 2 inode bitmap
}

/**
 * @function init_sect_bitmap
 * @brief 初始化sector bitmap
 * @param p_sb  超级块
 * @return
 */
void init_sect_bitmap(struct super_block * p_sb)
{
	int i,j;
	memset(fsbuf, 0, SECTOR_SIZE);
	int nr_sects = DEFAULT_FILE_SECTS_COUNT + 1;
				/*            |   |
				 *	      |   `bit 0 is reserved
				 *	      `----bit 1 for '/'
				 */
	for(i=0; i < nr_sects / 8; i++){
		fsbuf[i] = 0xFF;
	}	
	for(j=0; j < nr_sects % 8; j++){
		fsbuf[i] |= (1<<j);
	}
	WRITE_SECT(ROOT_DEV, 2 + p_sb->imap_sects_count);
	//设置区域sector bitmap项为0
	memset(fsbuf, 0, SECTOR_SIZE);
	for(i=1;i<p_sb->smap_sects_count;i++){
		WRITE_SECT(ROOT_DEV, 2 + p_sb->imap_sects_count + i);
	}
}

/**
 * @function init_inode_array
 * @brief 初始化inode array 区域
 * @param p_sb 超级块
 * @return
 */ 
void init_inode_array(struct super_block * p_sb)
{
	//inode for '/'
	memset(fsbuf, 0, SECTOR_SIZE);
	struct inode * pi = (struct inode *)fsbuf;
	pi->i_mode = I_DIRECTORY;
	pi->i_size = DIR_ENTRY_SIZE * 2;// '.', '..'
	pi->i_start_sect = p_sb->first_sect;
	pi->i_sects_count = DEFAULT_FILE_SECTS_COUNT; //暂时都设置成1KB 虽然有些浪费
	WRITE_SECT(ROOT_DEV, 2 + p_sb->imap_sects_count + p_sb->smap_sects_count);
}

/**
 * @function init_data_blocks
 * @brief 初始化文件数据区
 * @param p_sb 超级块
 * @return
 */
void init_data_blocks(struct super_block * p_sb)
{
	//初始化时只有/目录数据
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry *pde = (struct dir_entry*)fsbuf;
	pde->inode_idx = 1; //.
	strcpy(pde->name, ".");
	(++pde)->inode_idx = 1; //..
	strcpy(pde->name, "..");
	WRITE_SECT(ROOT_DEV, p_sb->first_sect);
	
}

/**
 * @function read_super_block
 * @brief Read super block from the given device then write it inot a free super_block[] slot.
 *
 * @param dev From which device the super block comes.
 *
 * @return void
 */
void read_super_block(int dev)
{
	int i;
	struct message msg;
	msg.type	= DEV_READ;
	msg.DEVICE	= MINOR(dev);
	msg.POSITION	= SECTOR_SIZE * 1; //sector 0 为boot sector 所以super block在sector 1
	msg.BUF		= fsbuf;
	msg.CNT		= SECTOR_SIZE;
	msg.PID		= TASK_FS;
	assert(dd_map[MAJOR(dev)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(dev)].driver_pid, &msg);
	//find a free slot in super_block[]
	for(i=0;i<MAX_SUPER_BLOCK_COUNT;i++){
		if(super_block[i].sb_dev == NO_DEV){
			break;
		}	
	}
	if(i==MAX_SUPER_BLOCK_COUNT){
		panic("super block slots used up");
	}
	assert(i==0);//currently we use only the 1st slot
	struct super_block *psb=(struct super_block*)fsbuf;
	super_block[i] = *psb;
	super_block[i].sb_dev = dev;
}

/**
 * @function get_super_block
 * @brief 从硬盘获取super block
 *
 * @param dev
 *
 * @return super_block  指针
 */
struct super_block * get_super_block(int dev)
{
	struct super_block *sb = super_block;
	for(;sb<super_block + MAX_SUPER_BLOCK_COUNT;sb++){
		if(sb->sb_dev == dev){
			return sb;
		}
	}
	panic("super block of devie %d not found.\n", dev);
	return 0;
}


/**
 * @function alloc_imap_bit
 * @brief 在inode-map中分配一位，这意味着新文件的inode有了确定的位置
 * @param dev
 * @return inode号
 */
int alloc_imap_bit(int dev)
{
	int inode_nr = 0; //return value
	int i, j, k;
	int imap_blk0_nr = 1 + 1;//1 boot sector & 1 superbloc
	struct super_block *sb = get_super_block(dev);
	for(i=0;i<sb->imap_sects_count;i++){
		READ_SECT(dev, imap_blk0_nr + i); //read from dev to fsbuf
		for(j=0;j<SECTOR_SIZE;j++){
			//skip 11111111 bytes
			if(fsbuf[j] == 0xFF) continue;
			//skip 1 bits
			for(k=0;((fsbuf[j]>>k)&1)!=0;k++){}
			//i:sector index;j:byte index;k:bit index
			inode_nr = (i*SECTOR_SIZE + j)*8+k;
			fsbuf[j] |= (1<<k);
			
			WRITE_SECT(dev, imap_blk0_nr + i); //write to dev from fsbuf
			break;
		}
		return inode_nr;
	}
	panic("inode-map is probably full.\n");
	return 0;
}


/**
 * @function alloc_smap_bit
 * @brief 在sector-map 中分配多位，这意味着为文件内容分配了扇区
 * @param dev In which device the sector-map is located.
 * @param sects_count_to_alloc how many  sectors are allocated.
 * @return The 1st sector nr allocated.
 */
int alloc_smap_bit(int dev, int sects_count_to_alloc)
{
	int i; //sector index
	int j; //byte index
	int k; //bit index
	
	struct super_block *sb = get_super_block(dev);
	int smap_blk0_nr = 1 + 1 + sb->imap_sects_count;
	int free_sect_nr = 0;
	for(i=0;i<sb->smap_sects_count;i++){
		READ_SECT(dev, smap_blk0_nr + i); //read data to fsbuf
		
		for(j=0;j<SECTOR_SIZE && sects_count_to_alloc > 0;j++){
			k = 0;
			if(!free_sect_nr){
				//loop until a free bit is fouond
				if(fsbuf[j]==0xFF) continue;
				for(;((fsbuf[j]>>k)&1)!=0;k++){}
				free_sect_nr = (i*SECTOR_SIZE + j)*8 + k-1+sb->first_sect;
			}
			for(;k<8;k++){
				//repeat till enough bits are set
				assert((fsbuf[j]>>k&1)==0);
				fsbuf[j] |= (1<<k);
				if(--sects_count_to_alloc==0){
					break;
				}
			}
		}
		if(free_sect_nr){ //free bit found, write the bits to smap
			WRITE_SECT(dev, smap_blk0_nr + i); //write fsbuf to dev
		}
		if(sects_count_to_alloc == 0){
			break;
		}
	}
	assert(sects_count_to_alloc == 0);
	return free_sect_nr;
}

/**
 * @function new_inode
 * @brief 在inode_array中分配一个inode并写入内容
 * @param dev home device of the inode
 * @param inode_nr
 * @param i_mode
 * @param start_sect
 * @param i_sects_count
 * @param i_size 
 *
 * @return ptr of the new inode
 */
struct inode* new_inode(struct inode *parent, int inode_nr,u32 i_mode, u32 start_sect, u32 i_sects_count, u32 i_size)
{
	//get from inode array by inode_nr
	struct inode * new_inode = get_inode(parent, inode_nr);
	new_inode->i_mode = i_mode;
	new_inode->i_size = i_size;
	new_inode->i_start_sect = start_sect;
	new_inode->i_sects_count = i_sects_count;
	
	new_inode->i_dev = parent->i_dev;
	new_inode->i_cnt = 1;
	new_inode->i_num = inode_nr;
	new_inode->i_parent = parent;
	
	//write4 to the inode array
	sync_inode(new_inode);
	return new_inode;
}

/**
 * @function syn_inode
 * @brief write the inode block to the disk.
 *  Commonly invoked as soon as the inode is changed.
 *
 * @param p inode ptr
 */
void sync_inode(struct inode *p)
{
	struct inode *pinode;
	struct super_block *sb = get_super_block(p->i_dev);
	int blk_nr = 1 + 1 + sb->imap_sects_count + sb->smap_sects_count + ((p->i_num - 1)/ (SECTOR_SIZE/INODE_SIZE));
	READ_SECT(p->i_dev, blk_nr);
	pinode = (struct inode*)((u8*)fsbuf + (((p->i_num - 1) % (SECTOR_SIZE/INODE_SIZE)) * INODE_SIZE));
	pinode->i_mode = p->i_mode;
	pinode->i_size = p->i_size;
	pinode->i_start_sect = p->i_start_sect;
	pinode->i_sects_count = p->i_sects_count;
	WRITE_SECT(p->i_dev, blk_nr); // READ_SECT 已经将pinode数据结构拷贝到fsbuf里了，这里将fsbuf写回磁盘
}

/**
 * @function create_file
 * @brief 创建新的inode，并设置磁盘上的数据
 * @param filename 文件名
 * @param dir_inode 文件所处目录inode
 * @param flags
 * 
 * @return 新inode指针，失败返回0
 */
//struct inode * create_file(char *path, int type,  int flags, int start_sect, int sect_count)
struct inode * create_file(struct inode * dir_inode, const char *filename, int types , int flags)
{
	int inode_nr = alloc_imap_bit(dir_inode->i_dev);
	int free_sect_nr = alloc_smap_bit(dir_inode->i_dev, DEFAULT_FILE_SECTS_COUNT);
	struct inode *newino = new_inode(dir_inode, inode_nr,type, free_sect_nr, DEFAULT_FILE_SECTS_COUNT, 0);
	new_dir_entry(dir_inode, newino->i_num, filename);
	return newino;
}

/**
 * @function create_special_file
 * @brief 创建新的inode，并设置磁盘上的数据
 * @param filename 文件名
 * @param dir_inode 文件所处目录inode
 * @param flags
 * 
 * @return 新inode指针，失败返回0
 */
struct inode * create_special_file(struct inode * dir_inode, const char * filename, int types, int flags, int dev)
{
	int inode_nr = alloc_imap_bit(dir_inode->i_dev);
	struct inode *newino = new_inode(dir_inode, inode_nr,type, dev, 0, 0);
	new_dir_entry(dir_inode, newino->i_num, filename);
	return newino;
}


/**
 * @function new_dir_entry
 * @brief 在相应的目录中写入一个目录项
 * @param dir_inode inode of the directory
 * @param inode_nr inode nr of the new file.
 * @param filename Filename of the new file.
 */
void new_dir_entry(struct inode *dir_inode, int inode_nr, const char *filename)
{
	//HOOK POINT
	if(dir_inode->i_dev == FLOPPYA_DEV){
		return new_dir_entry_fat12(dir_inode, inode_nr, filename);
	}
	//write the dir_entry
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE)/SECTOR_SIZE;
	int nr_dir_entries = dir_inode->i_size/DIR_ENTRY_SIZE;
	int m = 0;
	struct dir_entry * pde;
	struct dir_entry * new_de = 0;
	int i,j;
	int need_sync = 0;
	for(i=0;i<nr_dir_blks;i++){
		READ_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		pde = (struct dir_entry*)fsbuf;
		for(j=0;j<SECTOR_SIZE/DIR_ENTRY_SIZE; j++, pde++){
			if(++m>nr_dir_entries){
				break;
			}
			if(pde->inode_idx == 0){
				//it's a free slot
				new_de = pde;
				break;
			}
		}
		if(m>nr_dir_entries||new_de){//all entries have been iterated or free slot is fount
			break;
		}
	}
	if(!new_de){//reached the end of the dir
		new_de = pde;
		dir_inode->i_size += DIR_ENTRY_SIZE;
		need_sync = 1;
	}
	new_de->inode_idx = inode_nr;
	strcpy(new_de->name, filename);
	//write dir block 
	WRITE_SECT(dir_inode->i_dev, dir_blk0_nr + i);
	
	if(need_sync){
		//update dir inode
		sync_inode(dir_inode); //只有在dir_inode->size改变的时候才需要sync_inode
	}
}

/**
 * @function rm_dir_entry
 * @brief 清理inode_nr所在目录的目录项
 * @param dir_inode 所在目录的inode指针
 * @param inode_nr 文件的inode
 */
void rm_dir_entry(struct inode *dir_inode, int inode_nr)
{
	//HOOK POINT
	if(dir_inode->i_dev == FLOPPYA_DEV){
		return rm_dir_entry_fat12(dir_inode, inode_nr);
	}
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE)/SECTOR_SIZE;
	int nr_dir_entries = dir_inode->i_size /DIR_ENTRY_SIZE;
	int m = 0;
	struct dir_entry *pde = 0;
	int flag = 0;
	int dir_size = 0;
	for(i=0;i<nr_dir_blks;i++){
		READ_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		pde=(struct dir_entry*)fsbuf;
		int j;	
		for(j=0;j<SECTOR_SIZE/DIR_ENTRY_SIZE;j++,pde++){
			if(++m>nr_dir_entries) break;
			if(pde->inode_idx == inode_nr){
				memset(pde, 0, DIR_ENTRY_SIZE);
				WRITE_SECT(dir_inode->i_dev, dir_blk0_nr+i);
				flag = 1;
				break;
			}
			if(pde->inode_idx != INVALID_INODE){
				dir_size += DIR_ENTRY_SIZE;
			}
		}
		if(m>nr_dir_entries||flag){
			break;
		}
	}
	assert(flag);
	if(m==nr_dir_entries){
		//the file is the last one in the dir
		dir_inode->i_size = dir_size;
		sync_inode(dir_inode);
	}
}