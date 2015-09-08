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
/**
 * @function get_inode
 * @brief 根据inode_idx找到inode指针
 * @param parent 父目录inode ptr
 * @param inode_idx inode表的下表
 * @return inode 指针
 */
static struct inode * get_inode(struct inode *parent, int inode_idx);
static void sync_inode(struct inode *p);

static struct inode * create_file(struct inode *dir_inode ,const char * filename,  int flags);
static struct inode * create_special_file(struct inode *dir_inode ,const char * filename, int type,  int flags, int dev);
static struct inode * create_directory(struct inode * dir_inode, const char * filename, int flags);
static int is_dir_empty(struct inode * pinode);
// 新建dir entry 
static void new_dir_entry(struct inode *dir_inode, int inode_nr, const char *filename);
// 删除 dir entry
static void rm_dir_entry(struct inode *dir_inode, int inode_nr);

static int get_inode_num_from_dir(struct inode *parent,  const char * filename);

static int do_rdwt(int io_type, struct inode *pinode, void * buf, int pos , int len);

static int unlink_file(struct inode * pinode);

static void mount_dev(struct inode *pinode, int dev);

static void unmount_dev(struct inode *pinode, int newdev);

struct abstract_file_system tortoise = {
	ROOT_DEV,						//dev
	1,								//is_root
	init_tortoise_fs,				//init_fs_func
	get_inode,						//get_inode_func
	//sync_inode_fat12,				//sync_inode_func
	get_inode_idx_from_dir,			//get_inode_num_from_dir_func
	do_rdwt,						//rdwt_func
	create_file,					//create_file_func
	create_special_file,			//create_special_file_func
	create_directory,				//create_directory_func
	unlink_file,					//unlink_file_func
	is_dir_emtpy,					//is_dir_emtpy_func
	new_dir_entry,					//new_dir_entry_func
	rm_dir_entry,					//rm_dir_entry_func
	mount_dev,						//mount_func
	umount_dev,						//unmount_func
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
 * @function get_inode
 * @brief 根据inode号从inode array中返回inode指针
 * 
 * @param parent
 * @param inode_idx inode号
 *
 * @return The inode ptr requested
 */
struct inode * get_inode(struct inode *parent, int inode_idx)
{
	if(inode_idx==0){//0号inode没有使用
		return 0;
	}
	int dev;
	if(!parent){
		//root
		dev = ROOT_DEV;
	} else {
		dev = parent->i_dev;
	}
	//处理自己的文件系统
	struct inode * p;
	struct inode * q = 0;
	for(p= inode_table ; p<inode_table + MAX_INODE_COUNT; p++){
		if(p->i_cnt){
			//not a free slot
			//遇到挂载点会有问题，挂载过的目录的inode->i_dev已经被改变过了
			//这里暂时采用如下办法判断dev(目录dev) != inode->dev的情况：
			//		被挂载的目录，一定是根目录，那么根目录的inode idx值，就一定是ROOT_INODE
			if((p->i_dev == dev || parent->i_num == ROOT_INODE /* 只有挂载点会出现这种情况 */) && (p->i_num == inode_idx) && (p->i_parent == parent)){
				//this is the inode we want
				p->i_cnt ++ ;
				return p;
			}
		} else {
			//a free slot
			if(!q){
				//q hasn;t been assigned yet
				q=p;//q<-the 1st free slot
			}
		}
	}

	if(!q){
		panic("the inode able is full");
	}
	q->i_dev = dev;
	q->i_num = inode_idx;
	q->i_cnt = 1;
	q->i_parent = parent;
	struct super_block * sb = get_super_block(dev);
	int blk_nr = 1 + 1 + sb->imap_sects_count + sb->smap_sects_count + ((inode_idx-1)/(SECTOR_SIZE/INODE_SIZE));
	READ_SECT(dev, blk_nr);
	struct inode * pinode = (struct inode*)((u8*)fsbuf + ((inode_idx-1)%(SECTOR_SIZE/INODE_SIZE))*INODE_SIZE);
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_sect = pinode->i_start_sect;
	q->i_sects_count = pinode->i_sects_count;
	return q;
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
struct inode * create_file(struct inode * dir_inode, const char *filename, int flags)
{
	int inode_nr = alloc_imap_bit(dir_inode->i_dev);
	int free_sect_nr = alloc_smap_bit(dir_inode->i_dev, DEFAULT_FILE_SECTS_COUNT);
	struct inode *newino = new_inode(dir_inode, inode_nr,I_REGULAR, free_sect_nr, DEFAULT_FILE_SECTS_COUNT, 0);
	new_dir_entry(dir_inode, newino->i_num, filename);
	return newino;
}

struct inode * create_directory(struct inode *parent, const char *filename, int flags)
{
	int inode_nr = alloc_imap_bit(dir_inode->i_dev);
	int free_sect_nr = alloc_smap_bit(dir_inode->i_dev, DEFAULT_FILE_SECTS_COUNT);
	struct inode * newino = new_inode(dir_inode, inode_nr, I_DIRECTORY, free_sect_nr, DEFAULT_FILE_SECTS_COUNT, 2*DIR_ENTRY_SIZE);
	//写入directory文件数据 . ..
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry *pde;
	pde=(struct dir_entry*)fsbuf;
	pde->inode_idx = inode_nr;
	strcpy(pde->name, "."); //.
	(++pde)->inode_idx = dir_inode->i_num ;
	strcpy(pde->name, ".."); //..
	//写入磁盘
	WRITE_SECT(dir_inode->i_dev, free_sect_nr);	
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


int get_inode_num_from_dir(struct inode *parent,  const char * filename)
{
	int i;
	struct dir_entry *pde;
	int dir_entry_count_per_sect = SECTOR_SIZE/DIR_ENTRY_SIZE;
	int dir_entry_count = parent->i_size/DIR_ENTRY_SIZE;
	int dir_entry_blocks_count = parent->i_size/SECTOR_SIZE + parent->i_size%SECTOR_SIZE==0?0:1;
	for(i=0;i<dir_entry_blocks_count;i++){ 
		READ_SECT(parent->i_dev, parent->i_start_sect + i);
		pde=(struct dir_entry*)fsbuf;
		//int dir_entry_count = (*ppinode)->i_size/DIR_ENTRY_SIZE;
		int step = min(dir_entry_count_per_sect, dir_entry_count);
		for(j=0;j<step;j++, pde++){
			if(strcmp(pde->name, filename)==0){
				return pde->inode_idx;
			}
		}
		dir_entry_count -= dir_entry_count_per_sect;
	}
	return INVALID_INODE;
}

int do_rdwt(int io_type, struct inode *pin, void * buf, int pos , int len)
{
	int pos_end;
	if(io_type == DEV_READ){
		pos_end = min(pos+len, pin->i_size);
	} else {
		pos_end = min(pos+len, pin->i_sects_count * SECTOR_SIZE);
	}
	
	int off = pos % SECTOR_SIZE;
	int rw_sect_min = pin->i_start_sect + (pos>>SECTOR_SIZE_SHIFT);
	int rw_sect_max = pin->i_start_sect + (pos_end>>SECTOR_SIZE_SHIFT);
	int chunk = min(rw_sect_max - rw_sect_min + 1, FSBUF_SIZE >> SECTOR_SIZE_SHIFT);
	int bytes_rw = 0;
	int bytes_left = len;
	int i;
	for(i=rw_sect_min;i<=rw_sect_max;i+=chunk){
		//read/write this amount of bytes every time
		int bytes = min(bytes_left, chunk * SECTOR_SIZE - off);
		rw_sector(DEV_READ, pin->i_dev, i*SECTOR_SIZE, chunk*SECTOR_SIZE, TASK_FS, fsbuf);
		if(p_msg->type == READ){
			memcpy((void*)va2la(src, buf+bytes_rw), (void*)va2la(TASK_FS, fsbuf+off), bytes);
		} else {
			//write
			memcpy((void*)va2la(TASK_FS, fsbuf+off), (void*)va2la(src, buf+bytes_rw), bytes);
			//TODO add file cache here
			rw_sector(DEV_WRITE,pin->i_dev, i*SECTOR_SIZE, chunk*SECTOR_SIZE, TASK_FS, fsbuf);
		}
		off = 0;
		bytes_rw += bytes;
		pcaller->filp[fd]->fd_pos += bytes;
		bytes_left -= bytes;
	}
	if(pcaller->filp[fd]->fd_pos > pin->i_size){
		//update inode::size
		pin->i_size = pcaller->filp[fd]->fd_pos;
		//write the updated inode back to disk
		sync_inode(pin);
	}
	return bytes_rw;
}


/**
 * @function unlink_file
 * @brief 清除文件/目录
 * @param pinode  文件inode指针
 * 
 * @return 0 successful
 */
int unlink_file(struct inode *pinode)
{
	int inode_idx = pinode->i_num;
	struct super_block *sb = get_super_block(pinode->i_dev);
	//free the bit in imap
	int byte_idx = inode_idx/8;
	int bit_idx = inode_idx % 8;
	assert(byte_idx<SECTOR_SIZE);//we have only one imap sector
	//read sector 2 (skip bootsect and superblk):
	READ_SECT(pinode->i_dev, 2);
	assert(fsbuf[byte_idx%SECTOR_SIZE]&(1<<bit_idx));
	fsbuf[byte_idx % SECTOR_SIZE] &= ~(1<<bit_idx);
	WRITE_SECT(pinode->i_dev, 2);
	//free the bits in smap
	bit_idx = pinode->i_start_sect - sb->first_sect + 1;
	byte_idx = bit_idx/8;
	int bits_left = pinode->i_sects_count;
	int byte_cnt = (bits_left - (8-(bit_idx % 8)))/8; 
	
	//current sector nr
	int s= 2 + sb->imap_sects_count + byte_idx/SECTOR_SIZE;//2:bootsect + superblk
	READ_SECT(pinode->i_dev, s);
	int i;
	//clear the first byte
	for(i=bit_idx % 8;(i<8)&&bits_left;i++,bits_left--){
		assert((fsbuf[byte_idx % SECTOR_SIZE]>>i&1)==1);
		fsbuf[byte_idx%SECTOR_SIZE] &= ~(1<<i);
	}
	//clear bytes from the second byte to the second to last
	int k;
	i=(byte_idx % SECTOR_SIZE) + 1;
	for(k=0;k<byte_cnt;k++,i++,bits_left-=8){
		if(i==SECTOR_SIZE){
			i=0;
			WRITE_SECT(pinode->i_dev, s);
			READ_SECT(pinode->i_dev, ++s);
		}
		assert(fsbuf[i]==0xFF);
		fsbuf[i] = 0;
	}

	//clear the last byte
	if(i==SECTOR_SIZE){
		i=0;
		WRITE_SECT(pinode->i_dev, s);
		READ_SECT(pinode->i_dev, ++s);
	}
	unsigned char mask = ~((unsigned char)(~0)<<bits_left);
	assert((fsbuf[i]&mask)==mask);
	fsbuf[i] &= (~0)<<bits_left;
	WRITE_SECT(pinode->i_dev, s);

	//clear the inode itself
	pinode->i_mode = 0;
	pinode->i_size = 0;
	pinode->i_start_sect = 0;
	pinode->i_sects_count = 0;
	sync_inode(pinode);
	//release slot in inode_table[]
	put_inode(pinode);
	//set the inode-nr to 0 in the directory entry
	rm_dir_entry(pinode->parent, inode_idx);
	return 0;
}

int is_dir_empty(struct inode *pinode)
{
	int dir_entry_count = pinode->i_size/DIR_ENTRY_SIZE;
	return dir_entry_count<=2;
}

void mount_dev(struct inode *pinode, int dev)
{
	//validate
	//首先打开设备文件
	struct message driver_msg;
	reset_msg(&driver_msg);
	driver_msg.type = DEV_OPEN;
	driver_msg.DEVICE=MINOR(dev);
	//dd_map[1] = TASK_FLOPPY
	//assert(MAJOR(dev)==1); //可能有多个块设备
	assert(dd_map[MAJOR(dev)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(dev)].driver_pid, &driver_msg);
	assert(driver_msg.type==SYSCALL_RET);
	//dev已经被打开了
	//将target_pinode->i_dev改成挂在的设备文件dev
	//注意i_dev并不会持久化到磁盘
	//但是mount操作后，由于目录inode->i_cnt会增加，目标目录的inode会被缓存在内存中
	//所以可以在这里进行修改dev的操作
	//////////////////
	//这里的逻辑有问题
	//直接修改i_dev是错误的，因为挂载的设备跟我们的硬盘格式不同
	//////////////////
	pinode->i_dev = dev; //下次再打开挂在目录下的文件的时候，文件的dev就会因为从父目录中获取，从而改编成挂在的dev了
	//挂载同时还需要读取软盘BPB
	init_fat12_bpb(dev);
	//target_pinode的其他属性也要修改，如所在设备开始扇区号
	
	//TODO 获取目录所在扇区及大小
	//FAT12软盘中存储的结构与硬盘中的结构不同，所以需要在inode_table中开辟一个inode节点，虚拟的表示floppy中的文件
	//为了简化起见，fat12只允许一级目录，根目录下的目录将会被忽略
	struct BPB * bpb_ptr = FAT12_BPB_PTR(dev);
	pinode->i_start_sect = bpb_ptr->rsvd_sec_cnt + bpb_ptr->hidd_sec + bpb_ptr->num_fats * (bpb_ptr->fat_sz16>0 ? bpb_ptr->fat_sz16 : bpb_ptr->tot_sec32);//【BPB结构中的(rsvd_sec_cnt + hidd_sec + num_fats* (fat_sz16>0?fat_sz16:tot_sec32))】 //root目录开始扇区
	//pinode->i_sects_count = bpb_ptr->root_ent_cnt * sizeof(struct fat12_dir_entry)/bpb_ptr->bytes_per_sec; //【root_ent_cnt*sizeof(struct RootEntry)/bytes_per_sec】 //根目录最大文件数*每个目录项所占字节数/每个扇区的字节数=占用扇区数
	pinode->i_size = bpb_ptr->root_ent_cnt * sizeof(struct fat12_dir_entry); //【root_ent_cnt*sizeof(struct RootEntry)】
	pinode->i_sects_count = pinode->i_size / bpb_ptr->bytes_per_sec;
	pinode->i_num = ROOT_INODE;			//【说明】本来预计floppy文件的inode值就设置成第一个簇号
											//但是为了与硬盘的根目录统一，这里就设置成ROOT_INODE也就是1了
											//其他floppy文件可以设置成开始簇号
											//inode_table可以看作一个map (dev, inode)->pinode  所以不同的dev下，可以用相同的inode号
											//另外挂载点的i_num设置成ROOT_INODE,在 @function get_inode 方法中也可以用来做【正确】的判断
	//////
	//下次再进入target目录后，发现i_dev != ROOT_DEV，说明被挂载了其他设备
	//就不能再按现有的方式读取directory_entry，进而获取目录下的文件了
	//而是应该按照设备（比如软盘）的格式，读取根目录项
	//所以 strip_path  search_file 等地方都要修改，根据dev进行不同的操作
	//////
	//设备文件没必要占用inode table ,这里可以清空了
}

void unmount_dev(struct inode *pinode, int dev)
{
	//由于是根文件系统，所以不会被卸载
}