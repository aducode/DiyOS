/******************************************************/
/*                 操作系统的文件系统                 */
/******************************************************/
//现在处理起来有些问题：
//1. 最开始设计的文件系统只有 / 一级目录，所以close的时候只对文件的inode->i_cnt自减， 而如今变成多级树形目录，在open过程中会对路径上所有的目录文件的i_cnt自增，所以应该修改close函数，对所有目录文件inode->i_cnt自减
//	 在struct inode中添加了一个i_parent属性，用来保存父目录inode ptr , put_inode会递归的追溯i_parent进行清理
//2. 大量地方用到了strip_path 获取文件名和目录inode指针，同时用search_file获取文件的inode idx，而search_file中也使用了strip_path函数，这会导致i_cnt多加了一次，应该重新设计一下这两个函数，避免出现这样的错误
//	 合并search_file 和 strip_path ,统一使用search_file函数，调用完search_file后，根据具体需求调用put_inode
//要用到search_file的函数有:
//		do_open ：调用之后如果失败，需要CLEAR_INODE， 否则不需要CLEAR_INODE，一直保存inode直到对应fd调用do_close()
//		do_unlink: 调用之后无论是否失败，都需要CLEAR_INODE
//		do_rmdir : 调用之后无论失败，都需要CLEAR_INODE
//		do_stat:	调用之后无论失败，都需要CLEAR_INODE
#include "type.h"
#include "syscall.h"
#include "global.h"
#include "assert.h"
#include "hd.h"
#include "fs.h"
#include "stdio.h"
#include "proc.h"
static void init_fs();
static void init_super_block(struct super_block * p_sb);
static void init_inode_bitmap();
static void init_sect_bitmap(struct super_block * p_sb);
static void init_inode_array(struct super_block * p_sb);
static void init_data_blocks(struct super_block * p_sb);
static void init_tty_files(struct inode *pin);
static void init_block_dev_files(struct inode *pin);
static void fmtfs();
static void read_super_block(int dev);
static struct super_block * get_super_block(int dev);
/**
 * @function get_inode
 * @brief 根据inode_idx找到inode指针
 * @param parent 父目录inode ptr
 * @param inode_idx inode表的下表
 * @return inode 指针
 */
static struct inode * get_inode(struct inode *parent, int inode_idx);
static void put_inode(struct inode *pinode);
static void sync_inode(struct inode *p);
static int do_open(struct message *p_msg);
static int do_close(struct message *p_msg);
static int do_rdwt(struct message *p_msg);
static int do_seek(struct message *p_msg);
static int do_tell(struct message *p_msg);
static int do_unlink(struct message *p_msg);
static int do_mkdir(struct message *p_msg);
static int do_rmdir(struct message *p_msg);
static int do_stat(struct message *p_msg);
static int do_mount(struct message *p_msg);
static int do_unmount(struct message *p_msg);
static int fs_fork(struct message *msg);
static int fs_exit(struct message *msg);
//只有在do_open中创建文件的时候使用
//static struct inode * create_file(char *path, int flags);
//使用strip_path返回的结果，避免重复调用strip_path
static struct inode * create_file(const char *filename, struct inode * dir_inode, int flags);
static struct inode * create_directory(char *path, int flags);
static int unlink_file(struct inode * pinode, struct inode* dir_inode);
static int alloc_imap_bit(int dev);
static int alloc_smap_bit(int dev, int sects_count_to_alloc);
static struct inode* new_inode(struct inode *parent, int inode_nr, u32 i_mode, u32 start_sect,u32 i_sects_count, u32 i_size);
static void new_dir_entry(struct inode *dir_inode, int inode_nr, const char *filename);
static int strip_path(char *filename, const char *pathname, struct inode **ppinode);
//添加参数，避免重复调用strip_path
static int search_file(const char *path, char * filename, struct inode **ppinode);

/**
 * @define CLEAR_INODE
 * @brief
 * @param dir_inode
 * @param inode
 */
#define CLEAR_INODE(dir_inode, inode)  do { \
	if((inode) != 0){ \
		put_inode((inode)); \
	} else if((dir_inode) != 0){ \
		put_inode((dir_inode)); \
	}										\
}while(0)

/**
 * Main Loop 
 */
void task_fs()
{
	init_fs();
	struct message msg;
	while(1){
		//wait for other process
		send_recv(RECEIVE, ANY, &msg);
		int src = msg.source;
		switch(msg.type){
			case OPEN:
				//open 要返回FD
				msg.FD = do_open(&msg);
				break;
			case CLOSE:
				//返回是否成功
				msg.RETVAL = do_close(&msg);
				break;
			case READ:
			case WRITE:
				//读写文件
				msg.CNT = do_rdwt(&msg);
				break;
			case STAT:
				//获取文件状态
				msg.RETVAL = do_stat(&msg);
				break;
			case SEEK:
				//seek
				msg.RETVAL = do_seek(&msg);
				break;
			case TELL:
				//tell
				msg.POSITION = do_tell(&msg);
				break;
			case UNLINK:
				//删除普通文件
				msg.RETVAL = do_unlink(&msg);
				break;
			case MKDIR:
				//创建空目录
				msg.RETVAL = do_mkdir(&msg);
				break;
			case RMDIR:
				//删除空目录
				msg.RETVAL = do_rmdir(&msg);
				break;
			case MOUNT:
				//挂载文件系统
				msg.RETVAL = do_mount(&msg);
				break;
			case UNMOUNT:
				//卸载文件系统
				msg.RETVAL = do_unmount(&msg);
				break;
			case RESUME_PROC:
				//恢复挂起的进程
				src = msg.PID; //恢复进程,此时将src变成TTY进程发来的消息中的PID
				break;
			case FORK:
				//fork 让子进程共享父进程的文件fd
				msg.RETVAL = fs_fork(&msg);
				break;
			case EXIT:
				//子进程退出时的文件操作
				msg.RETVAL = fs_exit(&msg);
				break;
			default:
				panic("invalid msg type:%d\n", msg.type);
				break;
		}
		//返回
		if(msg.type != SUSPEND_PROC){
			msg.type = SYSCALL_RET;
			send_recv(SEND, src, &msg);
		}
		//如果收到的msg.type == SUSPEND_PROC那么直接继续循环
	}
}

/**
 * @function init_fs
 * @brief 初始化文件系统
 */
void init_fs()
{
	int i;
	//f_desc_table[]
	for(i=0;i<MAX_FILE_DESC_COUNT;i++){
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));
	}
	//inode_table[]
	for(i=0;i<MAX_INODE_COUNT;i++){
		memset(&inode_table[i], 0, sizeof(struct inode));
	}
	//super_block[]
	struct super_block *sb = super_block;
	for(;sb<&super_block[MAX_SUPER_BLOCK_COUNT];sb++){
		sb->sb_dev = NO_DEV;
	}
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
	sb = (struct super_block*)fsbuf;
	//打开后创建文件系统
	fmtfs();
	//load super block of ROOT
	read_super_block(ROOT_DEV);
	sb=get_super_block(ROOT_DEV);
	assert(sb->magic == MAGIC_V1);
	//创建/dev目录
	struct inode *p_dir_inode = create_directory("/dev", O_CREATE);
	assert(p_dir_inode!=0);
	init_tty_files(p_dir_inode);
	//init block device files
	init_block_dev_files(p_dir_inode);
	//put_inode(p_dir_inode);
	assert(p_dir_inode->i_cnt == 0);
}


/**
 * @function int_tty_files
 * @brief 初始化tty设备文件
 * @return
 */
void init_tty_files(struct inode *dir_inode)
{
	//step 1 创建tty0 tty1 tty2三个tty设备文件
	char filename[MAX_PATH];
	int i, inode_nr, free_sect_nr;
	for(i=0;i<CONSOLE_COUNT;i++){
		struct inode *newino = 0;
		inode_nr = alloc_imap_bit(dir_inode->i_dev);
        	newino = new_inode(dir_inode, inode_nr,I_CHAR_SPECIAL, MAKE_DEV(DEV_CHAR_TTY, i),0, 0);
		sprintf(filename, "tty%d", i);
       		new_dir_entry(dir_inode, newino->i_num, filename);
		assert(inode_nr!=0 && newino != 0);
		put_inode(newino);
	}
		
}

/**
 * @function init_block_dev_files
 * @brief 初始化块设备文件
 * @return
 */
void init_block_dev_files(struct inode *dir_inode)
{
	int inode_nr, free_sect_nr;
	struct inode *newino = 0;
	inode_nr = alloc_imap_bit(dir_inode->i_dev);
	newino = new_inode(dir_inode, inode_nr, I_BLOCK_SPECIAL, MAKE_DEV(DEV_FLOPPY, 0), 0, 0);
	new_dir_entry(dir_inode, newino->i_num, "floppy");
	assert(inode_nr != 0 && newino != 0);
	put_inode(newino);
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
 * @function rw_sector
 * R/W a sector via messaging with the corresponding driver
 *
 * @param io_type	DEV_READ or DEV_WRITE
 * @param dev		device number
 * @param pos		Byte offset from/to where to r/w
 * @param size		How many bytes
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



/**
 * @function do_open
 * @brief open file
 * @param p_msg msg address
 *
 * @return File descriptor if successful, otherwise a negative error code.
 */
int do_open(struct message *p_msg)
{
	int fd = -1;
	char pathname[MAX_PATH];
	int flags = p_msg->FLAGS;
	int name_len = p_msg->NAME_LEN;
	int src = p_msg->source;
	struct process *pcaller = proc_table + src;
	assert(name_len<MAX_PATH);
	memcpy((void*)va2la(TASK_FS, pathname), (void*)va2la(src, p_msg->PATHNAME), name_len);
	pathname[name_len] = 0;
	//find a free slot in PROCESS:filp[]
	int i;
	for(i=0;i<MAX_FILE_COUNT;i++){
		if(pcaller->filp[i] == 0){
			fd = i;
			break;
		}
	}
	if((fd<0)||(fd>=MAX_FILE_COUNT)){
		panic("filp[] is full (PID:%d)", src);
	}
	//find a free slot in f_desc_table[]
	for(i=0;i<MAX_FILE_DESC_COUNT;i++){
		if(f_desc_table[i].fd_inode == 0){
			break;
		}
	}
	if(i>=MAX_FILE_DESC_COUNT){
		panic("f_desc_table[] is full (PID:%d)", src);
	}
	char filename[MAX_PATH];
	struct inode *dir_inode = 0, *pin=0;
	int inode_nr = search_file(pathname, filename, &dir_inode);
	if(inode_nr == -1){
		goto label_fail;
	}
	//原来这里是这样写的，但是上面inode_nr==-1的时候直接跳到label_fail
	//而下面pin变量并没有被初始化，所以label_fail中对应的pin变量的值是未知的
	//会产生问题
	//struct inode *pin = 0;
	if(flags & O_CREATE){
		//创建文件
		if(inode_nr>0){
			//printk("file exists.\n");
			//put_inode(dir_inode);
			//return -1;
			goto label_fail;
		} else {
			pin = create_file(filename, dir_inode, flags);
		}
	} else {
		assert(flags & O_RDWT);
		if(inode_nr==0){
			//printk("file does not exist.\n");
			//return -1;
			goto label_fail;
		}
		pin = get_inode(dir_inode, inode_nr);
	}
	if(pin){
		if(pin->i_mode == I_DIRECTORY){
			//不能打开目录文件
			//释放inode table中的缓存
			//put_inode(pin);
			//return -1;
			goto label_fail;
		}
		//connects proc with file_descriptor
		pcaller->filp[fd] = &f_desc_table[i];
		//connects file_descriptor with inode
		f_desc_table[i].fd_inode = pin;
		f_desc_table[i].fd_mode = flags;
		f_desc_table[i].fd_pos = 0;
		f_desc_table[i].fd_cnt = 1;
		int imode = pin->i_mode & I_TYPE_MASK;
		if(imode == I_CHAR_SPECIAL) {
			//设备文件 dev_tty0 dev_tty1 dev_tty2
			struct message driver_msg;
			driver_msg.type = DEV_OPEN;
			int dev = pin->i_start_sect;
			driver_msg.DEVICE=MINOR(dev);
			//@see kernel/global.c dd_map[]
			//4 is index of dd_map
			//dd_map[4] = TASK_TTY
			assert(MAJOR(dev) == 4);
			assert(dd_map[MAJOR(dev)].driver_pid != INVALID_DRIVER);
			//printk("FS send message to %d\n", dd_map[MAJOR(dev)].driver_pid);
			send_recv(BOTH, dd_map[MAJOR(dev)].driver_pid, &driver_msg);
			assert(driver_msg.type==SYSCALL_RET);
		} else if (imode == I_BLOCK_SPECIAL) {
			//打开块设备文件
			//之前考虑open块设备时发送DEV_OPEN消息
			//现在改为mount时发送DEV_OPEN消息
			/*
			struct message driver_msg;
			driver_msg.type = DEV_OPEN;
			int dev = pin->i_start_sect;
			driver_msg.DEVICE=MINOR(dev);
			//dd_map[1] = TASK_FLOPPY
			//assert(MAJOR(dev)==1); //可能有多个块设备
			assert(dd_map[MAJOR(dev)].driver_pid != INVALID_DRIVER);
			send_recv(BOTH, dd_map[MAJOR(dev)].driver_pid, &driver_msg);
			assert(driver_msg.type==SYSCALL_RET);
			*/
			assert(imode == I_BLOCK_SPECIAL);
		} else if (imode == I_DIRECTORY) {
			//assert(pin->i_num == ROOT_INODE);
			assert(0); //不会到这里
		} else {
			assert(imode == I_REGULAR);
		}
	} else {
		//return -1;
		goto label_fail;
	}
	return fd;
label_fail:
	CLEAR_INODE(dir_inode, pin);
	return -1;
}

/**
 * @function do_seek
 * @brief 
 */
int do_seek(struct message *p_msg)
{
	int fd = p_msg->FD;
	int offset = p_msg->OFFSET;
	int where = p_msg->WHERE;
	if((offset<0 && where == 0) || (offset>0 && where == 2)){
		//where:0 SEEK_START 偏移只能为正
		//where:2 SEEK_END   偏移只能为负
		return -1;
	}
	int src = p_msg->source;
	struct process *pcaller = proc_table + src;
        assert((pcaller->filp[fd]>=f_desc_table) & (pcaller->filp[fd] < f_desc_table + MAX_FILE_DESC_COUNT));
        if(!(pcaller->filp[fd]->fd_mode & O_RDWT)){
                return -1;
        }
	struct inode *pin = pcaller->filp[fd]->fd_inode;
	assert(pin>=inode_table && pin<inode_table + MAX_INODE_COUNT);
	if(pin->i_cnt == 0) {
		//说明fd没有对应的inode
		return -1;
	}
	int imode = pin->i_mode & I_TYPE_MASK;
	if(imode!=I_REGULAR){
		return -1;//只对普通文件有效
	}
	int file_size = pin->i_size;	
        int pos = pcaller->filp[fd]->fd_pos;
	int new_pos = 0;
	switch(where){
	case 0:
		new_pos = offset;
		break;
	case 1:
		new_pos = pos + offset;
		break;
	case 2:
		new_pos = file_size + offset; //之前已经判断过 offset一定为负数
		break;
	default:
		break;
	}
	if(new_pos<0||new_pos>file_size){
		return -1; //invalid new pos
	}
	//update pos
	pcaller->filp[fd]->fd_pos = new_pos;
	return 0;	
}

/**
 * @function do_tell
 * @brief 文件位置
 */
int do_tell(struct message *p_msg)
{
	int fd= p_msg->FD;
	int src = p_msg->source;
	struct process *pcaller = proc_table + src;
    assert((pcaller->filp[fd]>=f_desc_table) & (pcaller->filp[fd] < f_desc_table + MAX_FILE_DESC_COUNT));
    if(!(pcaller->filp[fd]->fd_mode & O_RDWT)){
        return -1;
    }
	struct inode * pin = pcaller->filp[fd]->fd_inode;
    assert(pin>= inode_table && pin<inode_table + MAX_INODE_COUNT);
	if(pin->i_cnt == 0) {
		//说明fd没有对应的inode
		return -1;
	}
    int imode = pin->i_mode & I_TYPE_MASK;
	if(imode!=I_REGULAR){
		//只能对普通文件进行操作
		return -1;
	}
        int pos = pcaller->filp[fd]->fd_pos;
	return pos;
}

/**
 * @function do_close
 * @brief  关闭文件
 * 
 *  @param p_msg
 *  
 */
int do_close(struct message *p_msg)
{
	int fd = p_msg->FD;
	int src = p_msg->source;
	struct process *pcaller = proc_table + src;
	if(pcaller->filp[fd]->fd_inode->i_cnt == 0) {
		//说明fd没有对应的inode
		return -1;
	}
	//释放inode 资源
	put_inode(pcaller->filp[fd]->fd_inode);
	if(--pcaller->filp[fd]->fd_cnt == 0){
		pcaller->filp[fd]->fd_inode = 0;
	}
	//清空filp指向的f_desc_table中某一表项的fd_inode指针，归还f_desc_table的slot
	pcaller->filp[fd]->fd_inode = 0;
	//归还process中的filp slot
	pcaller->filp[fd] = 0;
	return 0;
}

/**
 * @function do_rdwt
 * @brief 读写文件
 * 
 * @param p_msg 消息指针
 *
 * @return 返回读写的字节数
 */
int do_rdwt(struct message *p_msg)
{
	int fd = p_msg->FD; //file descriptor
	void *buf = p_msg->BUF; //r/w buffer user's level
	int len = p_msg->CNT;//r/w bytes
	int src = p_msg->source;
	struct process *pcaller = proc_table + src;
	assert((pcaller->filp[fd]>=f_desc_table) & (pcaller->filp[fd] < f_desc_table + MAX_FILE_DESC_COUNT));
	if(!(pcaller->filp[fd]->fd_mode & O_RDWT)){
		return -1;
	}
	int pos = pcaller->filp[fd]->fd_pos;
	struct inode * pin = pcaller->filp[fd]->fd_inode;
	assert(pin>= inode_table && pin<inode_table + MAX_INODE_COUNT);
	int imode = pin->i_mode & I_TYPE_MASK;
	if(imode == I_CHAR_SPECIAL){
		int t = p_msg->type ==READ?DEV_READ:DEV_WRITE;
		p_msg->type = t;
		int dev = pin->i_start_sect;
		assert(MAJOR(dev) == 4);
		p_msg->DEVICE =  MINOR(dev);
		p_msg->BUF = buf;
		p_msg->CNT = len;
		p_msg->PID = src;
		assert(dd_map[MAJOR(dev)].driver_pid != INVALID_DRIVER);
		send_recv(BOTH, dd_map[MAJOR(dev)].driver_pid, p_msg);
		assert(p_msg->CNT == len);
		//assert(p_msg->type == SYSCALL_RET);
		//收到的消息不是都是SYSCALL_RET类型的，TTY read返回SUSPEND_PROC
		return p_msg->CNT;
	} else if(imode == I_BLOCK_SPECIAL){
		//读写块设备
		//这里设定可以读块设备内的raw数据
		//if(p_msg->type == READ) {
			//read函数理论上可以直接读取软盘
			//但是这没什么意义，而且read传入的count可以是任意Byte，而我们floppy的DEV_OPEN处理函数一般要求512Byte为单位
			//所以当对BLOCK 文件read/write时，直接忽略，不错操作
		//} else {
			//write函数中不可以直接写入软盘
		//}
		return 0; //直接返回0byte
	}else {
		assert(pin->i_mode == I_REGULAR || pin->i_mode == I_DIRECTORY);
		assert((p_msg->type == READ)||(p_msg->type == WRITE));
		int pos_end;
		if(p_msg->type == READ){
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
}




/**
 * @function do_unlink
 * @brief  释放inode slot
 *
 * @param p_msg
 *
 * @return 0 successful, -1 error
 */
int do_unlink(struct message *p_msg)
{
	char pathname[MAX_PATH];
	//get parameters from the message;
	int name_len = p_msg->NAME_LEN;
	int src = p_msg->source;
	assert(name_len<MAX_PATH);
	memcpy((void*)va2la(TASK_FS, pathname), (void*)va2la(src, p_msg->PATHNAME), name_len);
	pathname[name_len]=0;
	
	if(strcmp(pathname, "/") == 0){
		printk("FS:do_unlink():: cannot unlink the root\n");
		return -1;
	}
	
	char filename[MAX_PATH];
	struct inode *dir_inode = 0;
	struct inode *pin = 0;
	int inode_nr = search_file(pathname, filename, &dir_inode);
	if(inode_nr == INVALID_INODE || dir_inode == 0){
		//file not found
		printk("FS::do_unlink():: search_file() return invalid inode: %s\n", pathname);
		//return -1;
		goto label_fail;
	}
	pin = get_inode(dir_inode, inode_nr);
	if(pin->i_mode != I_REGULAR){
		//can only remove regular files
		//if you want remove directory  @see do_rmdir
		printk("cannot remove file %s, because it is not a regular file.\n", pathname);
		//return -1;
		goto label_fail;
	}
	if(pin->i_cnt>1){
		//thie file was opened
		printk("cannot remove file %s, because pin->i_cnt is %d.\n", pathname, pin->i_cnt);
		//return -1;
		goto label_fail;
	}
	//这里pin->i_cnt应该是1
	assert(pin->i_cnt == 1);
	//unlink_file内部会进行put_inode操作
	return unlink_file(pin, dir_inode);
label_fail:
	CLEAR_INODE(dir_inode, pin);
	return -1;
}


/**
 * @function do_mkdir
 * @brief 创建目录
 * @param p_msg
 * @return
 */
int do_mkdir(struct message *p_msg)
{
	char pathname[MAX_PATH];
    	int name_len = p_msg->NAME_LEN;
    	int src = p_msg->source;
    	assert(name_len<MAX_PATH);
    	memcpy((void*)va2la(TASK_FS, pathname), (void*)va2la(src, p_msg->PATHNAME), name_len);
    	pathname[name_len] = 0;
    	struct inode *dir_inode = create_directory(pathname, O_CREATE);
    	if(dir_inode == 0){
		//
		return -1;
	} else {
		put_inode(dir_inode);
		return 0;
	}
}

/**
 * @function do_rmdir
 * @brief 删除空目录
 * @param p_msg
 * @return
 */
int do_rmdir(struct message *p_msg)
{
	char pathname[MAX_PATH];
	int src = p_msg->source;
	int name_len = p_msg->NAME_LEN;
	assert(name_len<MAX_PATH);
	memcpy((void*)va2la(TASK_FS, pathname), (void*)va2la(src, p_msg->PATHNAME), name_len);
	pathname[name_len] = 0;
	if(strcmp(pathname, "/") == 0){
		return -1;//根目录不能删除
	}
	//只有这里使用的逻辑就不单独提出一个函数了
	char filename[MAX_PATH];
	struct inode *dir_inode = 0; //父目录inode指针
	int dir_entry_count; //目录项数量
	int inode_idx = search_file(pathname, filename, &dir_inode);
	if(inode_idx == INVALID_INODE || dir_inode == 0){
		//file not found
		//return -1;
		goto label_fail;
	}
	struct inode *pinode = get_inode(dir_inode, inode_idx);
	if(!pinode){
		//return -1;
		goto label_fail;
	}
	if(pinode->i_mode != I_DIRECTORY){
		//不是目录文件
		//return -1;
		goto label_fail;
	}
	if(pinode->i_cnt>1){
		//open函数不能用于目录文件，所以目录的pinode->i_cnt一般恒为1
		//但是考虑mount的时候可以将pinode->i_cnt 加1，所以这里也判断一下,防止删除挂载点
		printk("pathname:%s,inode:%d,i_cnt:%d\n", pathname,pinode->i_num,pinode->i_cnt);
		//return -1;
		goto label_fail;
	}
	//判断是否是空目录
	// 空目录是指只包含 . ..两个目录项的目录
	dir_entry_count = pinode->i_size/DIR_ENTRY_SIZE;
	if(dir_entry_count>2){
		//目录项大于2说明包含除了. ..之外的，所以不是空目录
		//return -1;
		goto label_fail;
	}
	//删除空目录
	//unlink_file 内部会进行put_inode 操作
	return unlink_file(pinode, dir_inode);
label_fail:
	CLEAR_INODE(dir_inode, pinode);
	return -1;
}
	
	
/**
 * @function unlink_file
 * @brief 清除文件/目录
 * @param pinode  文件inode指针
 * @param dir_inode 文件所在目录的inode指针
 * 
 * @return 0 successful
 */
int unlink_file(struct inode *pinode, struct inode* dir_inode)
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
			if(pde->inode_idx == inode_idx){
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
	return 0;
}
/**
 * @function fs_fork
 * @brief Perform the aspects of fork() that relate to files
 * @param msg message ptr
 * @return Zero if success, otherwise a negative integer.
 */
int fs_fork(struct message *msg)
{
	int i;
	struct process *child = &proc_table[msg->PID];
	for(i=0;i<MAX_FILE_COUNT;i++){
		if(child->filp[i]){
			child->filp[i]->fd_cnt++;
			child->filp[i]->fd_inode->i_cnt++;
		}
	}
	return 0;
}

/**
 * @function fs_exit
 * @brief  Perform the aspects of exit() that relate to files
 *
 */
 
int fs_exit(struct message *msg)
{
	int i;
	struct process *p = &proc_table[msg->PID];
	for(i=0;i<MAX_FILE_COUNT;i++){
		//release the inode
		p->filp[i]->fd_inode->i_cnt--;
		if(--p->filp[i]->fd_cnt == 0){
			p->filp[i]->fd_inode = 0;
		}
		p->filp[i] = 0;
	}
	return 0;
}


/**
 * @function create_directory
 * @brief 创建新的目录inode, 并设置磁盘上的数据
 * @param path 目录
 * @param  flags 预留
 * @return inode指针
 */
struct inode * create_directory(char *path, int flags)
{
	char filename[MAX_PATH];
	struct inode * dir_inode = 0;
	struct inode * pinode= 0;
	int inode_idx = search_file(path, filename, &dir_inode);
	if(inode_idx == -1){
		//说明目录链断开
		//创建目录的位置非法
		goto label_fail;		
	}
	if(inode_idx!=0){
		pinode = get_inode(dir_inode, inode_idx);
		if(pinode==0 || dir_inode == 0){
			//return 0;
			goto label_fail;
		}
		if(pinode->i_mode == I_DIRECTORY){
			return pinode; //说明已经存在目录
		} else {
			//return 0; //父目录下有同名文件
			goto label_fail;
		}
	}
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
label_fail:
	CLEAR_INODE(dir_inode, pinode);
	return 0;
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
struct inode * create_file(const char *filename, struct inode *dir_inode , int flags)
{
	int inode_nr = alloc_imap_bit(dir_inode->i_dev);
	int free_sect_nr = alloc_smap_bit(dir_inode->i_dev, DEFAULT_FILE_SECTS_COUNT);
	struct inode *newino = new_inode(dir_inode, inode_nr,I_REGULAR, free_sect_nr, DEFAULT_FILE_SECTS_COUNT, 0);
	new_dir_entry(dir_inode, newino->i_num, filename);
	return newino;
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
 * @function new_dir_entry
 * @brief 在相应的目录中写入一个目录项
 * @param dir_inode inode of the directory
 * @param inode_nr inode nr of the new file.
 * @param filename Filename of the new file.
 */
void new_dir_entry(struct inode *dir_inode, int inode_nr, const char *filename)
{
	//write the dir_entry
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE)/SECTOR_SIZE;
	int nr_dir_entries = dir_inode->i_size/DIR_ENTRY_SIZE;
	int m = 0;
	struct dir_entry * pde;
	struct dir_entry * new_de = 0;
	int i,j;
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
	}
	new_de->inode_idx = inode_nr;
	strcpy(new_de->name, filename);
	//write dir block 
	WRITE_SECT(dir_inode->i_dev, dir_blk0_nr + i);
	
	//update dir inode
	sync_inode(dir_inode);
}


/**
 * @function strip_path
 * @brief Get the basename from the fullpath
 *  in current DIYOS v0.2 FS v1.0, all files are stored in the root directory.
 *  there is no sub-folder thing
 *  this routine should be called at the very beginning of file operations
 *  such as open(), read() and write(). It accepts the full path and returns
 *  tow things: the basename and a ptr of the root dir's i-node
 * 
 * e.g. After strip_path(filename, "/blah", ppinode) finishes, we get:
 *	- filename: "blah"
 *	- *ppinode: root_inode
 *	- ret val: 0(successful)
 *
 * currently an acceptable pathname should begin with at most one '/'
 * preceding a filename.
 *
 * Filenames may contain any character except '/' and '\\0'
 *
 * It can work correctlly when path contains "." or ".."
 *
 * @param[out] filename the string for the result
 * @param[in] pathname the full pathname.
 * @param[out] ppinode the ptr of the dir's inode will be stored here.
 *
 * @return dir_inode_idx if success, otherwise the pathname is not valid.
 */
int strip_path(char *filename, const char *pathname, struct inode **ppinode)
{
	const char * s = pathname;
	struct dir_entry *pde;
	struct inode* pinode;
	char * t;
	int i,j;
	if(*s==0){
		//空字符串非法
		return -1;
	}
	if(*s!='/'){
		//必须是绝对路径
		return -1;
	}
	s++;
	*ppinode=root_inode();
	int dir_entry_count_per_sect = SECTOR_SIZE/DIR_ENTRY_SIZE;
	int dir_entry_count, dir_entry_blocks_count;
	while(*s!=0){
		pinode = 0;
		t = filename;
		while(*s!='/' && *s!=0){
			*t++=*s++;
		}
		*t=0;
		if(*s!=0) s++; //skip /
		dir_entry_count = (*ppinode)->i_size/DIR_ENTRY_SIZE;
		dir_entry_blocks_count = (*ppinode)->i_size/SECTOR_SIZE + (*ppinode)->i_size%SECTOR_SIZE==0?0:1;
		for(i=0;i<dir_entry_blocks_count;i++){ 
			READ_SECT((*ppinode)->i_dev, (*ppinode)->i_start_sect + i);
			pde=(struct dir_entry*)fsbuf;
			int dir_entry_count = (*ppinode)->i_size/DIR_ENTRY_SIZE;
			int step = min(dir_entry_count_per_sect, dir_entry_count);
			for(j=0;j<step;j++, pde++){
				if(strcmp(pde->name, filename)==0){
					pinode=get_inode(*ppinode, pde->inode_idx);
					goto try_to_find_next_path;		
				}
			}
			dir_entry_count -= dir_entry_count_per_sect;	
		}
try_to_find_next_path:
		if(*s!=0){
			//不是最后一级目录
			if(pinode==0){
				//1.目录不存在get_inode返回
				//2.没进入循环 初始值即为0
				//return -1;
				goto label_fail;
			}
			if(pinode->i_mode!=I_DIRECTORY){
				//不是目录
				//return -1;
				goto label_fail;
			}
			*ppinode=pinode;		
		} else {
			if(pinode!=0 && pinode->i_cnt>0){
				//需要将最后一层的i_cnt还原
				pinode->i_cnt--;
			}
		}
	}
	return 0;
label_fail:
	//put_inode(*ppinode); 不在这里清理，最外层函数统一清理
	return -1;
}



/**
 * @function search_file
 * @brief Search the file and return the inode_idx
 * @param[in] path the full path of the file to search
 * @param[out] filename the filename of the file to search
 * @param[out] ppinode the directory's inode ptr
 * @return Ptr to the inode of the file if successful
 *         Otherwise 0: 表示沿着目录链正确的找到dir_inode,但是目录项没有对应文件
 *		    -1: 表示没有沿着目录链找到正确的位置
 * @ses open()
 * @ses do_open()
 */
int search_file(const char *path, char *filename, struct inode **ppinode)
{
	int i,j;
	if(strip_path(filename, path, ppinode)!=0){
		return -1;
	}
	struct inode * dir_inode = *ppinode;
	if(filename[0] == 0){//path:"/"
		return dir_inode->i_num;
	}
	//search the dir for the file
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE-1)/SECTOR_SIZE;
	int nr_dir_entries = dir_inode->i_size/DIR_ENTRY_SIZE;
	
	int m = 0;
	struct dir_entry *pde;
	for(i=0;i<nr_dir_blks;i++){
		READ_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		pde = (struct dir_entry*)fsbuf;
		for(j=0;j<SECTOR_SIZE/DIR_ENTRY_SIZE;j++,pde++){
			if(strcmp(filename, pde->name)==0){
				return pde->inode_idx;
			}
			if(++m>nr_dir_entries){
				break;
			}
		}
		if(m>nr_dir_entries){
			break;
		}
	}
	return 0;//file not found
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
	struct inode * p;
	struct inode * q = 0;
	for(p= inode_table ; p<inode_table + MAX_INODE_COUNT; p++){
		if(p->i_cnt){
			//not a free slot
			if((p->i_dev == dev) && (p->i_num == inode_idx) && p->i_parent == parent){
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
 * @function put_inode
 * @brief Decrease the reference bumber of a slot in inode_table.
 * when the nr reaches zero  it means the inode is not used any more and can be overwritten by a new inode
 * @param pinode inode ptr
 */
void put_inode(struct inode *pinode)
{
	assert(pinode && pinode->i_cnt>0);	
	/*
	if(pinode==0 || pinode->i_cnt<=0){
		return;
	}
	*/
	pinode->i_cnt --;
	if(pinode->i_parent && pinode->i_parent->i_cnt>0){
		put_inode(pinode->i_parent);
	}
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
	WRITE_SECT(p->i_dev, blk_nr);
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
 * @function do_stat
 * @brief 处理stat消息
 * @param p_msg 消息体
 * @return 
 */
int do_stat(struct message *p_msg)
{
	int ret = -1;
	char pathname[MAX_PATH];
	//get parameters from the message;
	int name_len = p_msg->NAME_LEN;
	int src = p_msg->source;
	assert(name_len<MAX_PATH);
	memcpy((void*)va2la(TASK_FS, pathname), (void*)va2la(src, p_msg->PATHNAME), name_len);
	pathname[name_len]=0;
	char filename[MAX_PATH];
	struct inode *dir_inode = 0;
	struct stat stat; //内核层保存结果
	int inode_idx = search_file(pathname,filename, &dir_inode);
	if(inode_idx == INVALID_INODE || dir_inode == 0){
		//file not found
		printk("FS::do_stat():: search_file() return invalid inode: %s\n", pathname);
		ret = -1;
		goto label_clear_inode;
	}
	
	struct inode *pinode = 0;
	//TODO 这里get_inode有点问题，每次都是i_cnt==0导致每次都从磁盘重新load出来
	//inode_table[] 只是一个缓存，就算重新load，也不会影响i_size
	//而且奇怪的是随后的read还是可以读出数据的
	pinode = get_inode(dir_inode, inode_idx);
	if(!pinode){
		printk("FS::do_stat:: get_inode() return invalid inode: %s\n", pathname);
		ret = -1;
		goto label_clear_inode;
	}
	stat.st_dev = pinode->i_dev;
	stat.st_ino = pinode->i_num;
	stat.st_mode = pinode->i_mode;
	if(pinode->i_mode == I_CHAR_SPECIAL||pinode->i_mode == I_BLOCK_SPECIAL){
		stat.st_dev = pinode->i_start_sect;
	} else {
		stat.st_rdev = 0;
	}
	stat.st_size = pinode->i_size;
	memcpy((void*)va2la(src, p_msg->BUF), (void*)va2la(TASK_FS, &stat), sizeof(struct stat)); //数据拷贝到用户层的buf中
	ret = 0;
label_clear_inode:
	CLEAR_INODE(dir_inode, pinode);
	return ret;
}

/**
 * @function do_mount
 * @brief 处理MOUNT消息
 * @param p_msg
 * @return 
 */
int do_mount(struct message *p_msg)
{
	//TODO
	return -1;
}


/**
 * @function do_unmount
 * @brief 处理UNMOUNT消息
 * @param p_msg
 * @return 
 */
int do_unmount(struct message *p_msg)
{
	//TODO
	return -1;
}
