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
#include "floppy.h"
#include "fs.h"
#include "fat12.h"
#include "tortoise.h"
//#include "stdio.h"
#include "proc.h"

/**
 * @function get_afs
 * @brief 根据设备号获取抽象文件系统
 * @param dev 设备号
 * @return 抽象文件系统 ptr
 */
static struct abstract_file_system * get_afs(int dev);

static void init_fs();

static void init_tty_files(struct abstract_file_system * afs, struct inode *pin);
static void init_block_dev_files(struct abstract_file_system * afs, struct inode *pin);

static void put_inode(struct inode *pinode);

/** 文件操作 **/
static int do_open(struct message *p_msg);
static int do_close(struct message *p_msg);
static int do_rdwt(struct message *p_msg);
static int do_seek(struct message *p_msg);
static int do_tell(struct message *p_msg);
static int do_unlink(struct message *p_msg);
static int do_stat(struct message *p_msg);
static int do_rename(struct message *p_msg);
/** 目录操作 **/
static int do_mkdir(struct message *p_msg);
static int do_rmdir(struct message *p_msg);
static int do_chdir(struct message *p_msg);
/** 挂载操作 **/
static int do_mount(struct message *p_msg);
static int do_unmount(struct message *p_msg);
/** 辅助process fork **/
static int fs_fork(struct message *msg);
static int fs_exit(struct message *msg);
//只有在do_open中创建文件的时候使用
//static struct inode * create_file(char *path, int flags);
//使用strip_path返回的结果，避免重复调用strip_path
static struct inode * create_file(const char *filename, struct inode * dir_inode, int flags);
static struct inode * create_directory(char *path, int flags);
static int unlink_file(struct inode * pinode);

static int strip_path(char *filename, const char *pathname, struct inode **ppinode);
//添加参数，避免重复调用strip_path
static int search_file(const char *path, char * filename, struct inode **ppinode);

/**
 * @define CLEAR_INODE
 * @brief 清理目录树路径上的inode
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
				msg.FD = do_open(&msg);					//已经适配fat12
				break;
			case CLOSE:
				//返回是否成功
				msg.RETVAL = do_close(&msg);			//不需要适配fat12
				break;
			case READ:
			case WRITE:
				//读写文件
				msg.CNT = do_rdwt(&msg);				//已经适配fat12
				break;
			case STAT:
				//获取文件状态
				msg.RETVAL = do_stat(&msg);				//不需要适配fat12
				break;
			case SEEK:
				//seek
				msg.RETVAL = do_seek(&msg);				//不需要适配fat12
				break;
			case TELL:
				//tell
				msg.POSITION = do_tell(&msg);			//不需要适配fat12
				break;
			case UNLINK:
				//删除普通文件
				msg.RETVAL = do_unlink(&msg);			//已经适配fat12
				break;
			case RENAME:
				//重命名文件
				msg.RETVAL = do_rename(&msg);			//TODO
			case MKDIR:
				//创建空目录
				msg.RETVAL = do_mkdir(&msg);			//已经适配fat12
				break;
			case RMDIR:
				//删除空目录
				msg.RETVAL = do_rmdir(&msg);			//已经适配fat12
				break;
			case CHDIR:
				//修改进程所在目录
				msg.RETVAL = do_chdir(&msg);			//不需要适配fat12
				break;
			case MOUNT:
				//挂载文件系统
				msg.RETVAL = do_mount(&msg);			//因为只有rootfs和fat12两个文件系统，不存在嵌套挂载， 暂时不需要适配fat12
				break;
			case UNMOUNT:
				//卸载文件系统
				msg.RETVAL = do_unmount(&msg);			//因为只有rootfs和fat12两个文件系统，不存在嵌套挂载， 暂时不需要适配fat12
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


struct abstract_file_system * get_afs(int dev)
{
	struct abstract_file_system * ret;
	switch(dev){
	case ROOT_DEV:
		ret = &tortoise;
		break;
	case FLOPPYA_DEV:
		ret = &fat12;
		break;
	default:
		assert(0);
		break;
	}
	return ret;
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
	//init
	init_tortoise();
	init_fat12();
	//初始化根目录设备
	struct abstract_file_system * afs = get_afs(ROOT_DEV);
	afs->init_fs(ROOT_DEV);
	
	//创建/dev目录
	struct inode *dir_inode = root_inode();
	dir_inode = afs->create_file(dir_inode, "dev", I_DIRECTORY);
	//struct inode *p_dir_inode = create_directory("/dev", O_CREATE);
	//创建设备文件
	assert(afs->create_special_file != 0);
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
void init_tty_files(struct abstract_file_system * afs, struct inode *dir_inode)
{
	//step 1 创建tty0 tty1 tty2三个tty设备文件
	//tty字符设备一定存在，所以不需要检测
	char filename[MAX_PATH];
	int i
	struct inode * newino;
	for(i=0;i<CONSOLE_COUNT;i++){
		newino = 0;
		afs->create_file(dir_inode, )
		sprintf(filename, "tty%d", i);
		newino = afs->create_special_file(dir_inode, filename, I_CHAR_SPECIAL, O_CREATE, MAKE_DEV(DEV_CHAR_TTY, i));
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
	//软盘驱动设备也是一定存在的
	//软盘是否存在，要在挂载的时候判断
	struct inode *newino = 0;
	afs->create_special_file(dir_inode, "floppy", I_BLOCK_SPECIAL, O_CREATE, FLOPPYA_DEV);
	put_inode(newino);
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
		if(pin->i_dev == FLOPPYA_DEV){
			return do_rdwt_fat12(pin, buf, pos, len, src);
		}
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
	//if(inode_nr == INVALID_INODE || dir_inode == 0){ //dir_inode == 0的时候一定是返回INVALID_PATH的时候
	// 这里为了判断的统一，采用以下形式
	if(inode_nr == INVALID_INODE || inode_nr == INVALID_PATH){
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
	return unlink_file(pin);
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
	//if(inode_idx == INVALID_INODE || dir_inode == 0){
	if(inode_idx == INVALID_INODE || inode_idx == INVALID_PATH){
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
	//由于判读按目录是否为空在rootfs和fat12下的方法是不同的，所以即使在unlink_file中已经适配过fat12了，
	//还是要额外在这里判读一下是否是空目录
	//HOOK POINT
	switch(pinode->i_dev){
	case ROOT_DEV:
		//判断是否是空目录
		// 空目录是指只包含 . ..两个目录项的目录
		dir_entry_count = pinode->i_size/DIR_ENTRY_SIZE;
		if(dir_entry_count>2){
			//目录项大于2说明包含除了. ..之外的，所以不是空目录
			//return -1;
			goto label_fail;
		}
		break;
	case FLOPPYA_DEV:
		//fat12判读是否是空目录
		if(!is_dir_empty_fat12(pinode)){
			//不是空目录
			goto label_fail;
		}
		break;
	default:
		assert(0);
		break;
	}
	//删除空目录
	//unlink_file 内部会进行put_inode 操作
	return unlink_file(pinode);
label_fail:
	CLEAR_INODE(dir_inode, pinode);
	return -1;
}

/**
 * @function do_chdir
 * @brief 修改进程所在目录
 * @param p_msg
 * @return 0 if success
 */
int do_chdir(struct message *p_msg)
{
	char pathname[MAX_PATH];
	int src = p_msg->source;
	int name_len = p_msg->NAME_LEN;
	assert(name_len<MAX_PATH);
	memcpy((void*)va2la(TASK_FS, pathname), (void*)va2la(src, p_msg->PATHNAME), name_len);
	pathname[name_len] = 0;
	//
	char filename[MAX_PATH]; //出参参数
	struct inode *dir_inode = 0; //父目录inode指针
	int inode_idx;
	inode_idx = search_file(pathname, filename, &dir_inode);
	if(inode_idx == INVALID_INODE || inode_idx == INVALID_PATH){
		//无效的目录
		goto label_fail;
	}
	struct inode *pinode = get_inode(dir_inode, inode_idx);
	if(!pinode){
		goto label_fail;
	}
	if(pinode->i_mode != I_DIRECTORY){
		//不是目录
		goto label_fail;
	}
	//是一个合法目录
	struct process *pcaller = proc_table + src;
	//清空原来的current_path_inode
	if(!pcaller->current_path_inode){
		put_inode(pcaller->current_path_inode);
	}
	pcaller->current_path_inode = pinode; //设置新的进程目录
	return 0;
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
	//hook  for adapt fat12
	if(pinode->i_dev == FLOPPYA_DEV){
		return unlink_file_fat12(pinode); //fat12文件系统的unlink
	}
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
	//current_path_inode 也需要fork
	struct inode * pinode = child->current_path_inode;
	//init进程会保证设置current_path_inode,所以所有子进程都会有current_path_inode
	assert(pinode!=0)
	while(pinode != 0){
		pinode->i_cnt ++;
		pinode = pinode->parent;
	}
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
	//current_path_inode 也需要修改
	put_inode(p->current_path_inode);
	p->current_path_inode = 0;
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
	if(inode_idx == INVALID_PATH){
		//说明目录链断开
		//创建目录的位置非法
		goto label_fail;		
	}
	if(inode_idx!=INVALID_INODE){
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
	//HOOK POINT
	if(dir_inode->i_dev == FLOPPYA_DEV){
		//adapt for fat12
		return create_directory_fat12(dir_inode, filename);
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
	struct fat12_dir_entry *fat12pde;
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
	*ppinode=root_inode(); //从root inode开始寻找
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
		struct abstract_file_system * afs = get_afs((*ppinode)->i_dev);
		int inode_idx = afs->get_inode_num_from_dir(*ppinode, filename);
		if(inode_idx != INVALID_INODE){
			pinode = afs->get_inode(*ppinode, inode_idx);
		}
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
				//因为只有在search_file中会调用strip_path方法
				//而search_file最后还要对最后一级的文件进行cnt++操作
				//所以这里先还原
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
 * 			Otherwise INVAID_INODE (0): 表示沿着目录链正确的找到dir_inode,但是目录项没有对应文件
 * 					  INVALID_PATH (-1): 表示没有沿着目录链找到正确的位置
 * @ses open()
 * @ses do_open()
 */
int search_file(const char *path, char *filename, struct inode **ppinode)
{
	int i,j;
	if(strip_path(filename, path, ppinode)!=0){
		return INVALID_PATH;
	}
	struct inode * dir_inode = *ppinode;
	if(filename[0] == 0){//path:"/"
		return dir_inode->i_num;
	}
	return get_afs(dir_inode->i_dev)->get_inode_num_from_dir(dir_inode, filename);
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
	struct inode *dir_inode = 0, *pinode = 0;
	struct stat stat; //内核层保存结果
	int inode_idx = search_file(pathname,filename, &dir_inode);
	//if(inode_idx == INVALID_INODE || dir_inode == 0){
	if(inode_idx == INVALID_INODE || inode_idx == INVALID_PATH){
		//file not found
		printk("FS::do_stat():: search_file() return invalid inode: %s\n", pathname);
		ret = -1;
		goto label_clear_inode;
	}
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
 * @function do_rename
 * @brief 重命名文件
 * @param p_msg 消息体
 * @return 0 if success
 */
int do_rename(struct message *p_msg)
{
	int inode_idx = 0, ret = -1;
	struct inode *old_pinode = 0, *old_dir_inode = 0, *new_pinode = 0, *new_dir_inode = 0;
	char oldfilename[MAX_PATH]; //作为search_file 出参使用
	char newfilename[MAX_PATH]; //作为search_file 出参使用
	char oldname[MAX_PATH]; //内核进程空间内
	int oldname_len = p_msg->OLDNAME_LEN;
	assert(oldname_len < MAX_PATH);
	char newname[MAX_PATH]; //内核进程空间内
	int newname_len = p_msg->NEWNAME_LEN;
	assert(newname_len < MAX_PATH);
	//将数据从用户态复制到内核态
	int src = p_msg->source;
	memcpy((void*)va2la(TASK_FS, oldname), (void*)va2la(src, p_msg->OLDNAME), oldname_len);
	oldname[oldname_len] = 0;
	
	//判断oldname是否存在
	inode_idx = search_file(oldname, oldfilename,&old_dir_inode);
	//if(inode_idx == INVALID_INODE || old_dir_inode == 0){
	if(inode_idx == INVALID_INODE || inode_idx == INVALID_PATH){
		//oldname的目录存在错误
		goto label_clear_inode;
	}
	old_pinode = get_inode(old_dir_inode , inode_idx);
	if(!old_pinode){
		//oldname不存在
		goto label_clear_inode;
	}
	//判断是否是目录或者普通文件类型
	if(old_pinode->i_mode != I_DIRECTORY && old_pinode->i_mode != I_REGULAR){
		//不是目录也不是普通文件
		goto label_clear_inode;
	}
	// 考虑并发？TASK_FS 一次只能处理一个其他进程来的消息，所以对于文件的操作是串行化的，所以不需要考虑并发加锁
	// 但是要考虑已有其他进程打开过oldname文件的情况，这种情况应该立刻返回错误
	if(old_pinode->i_cnt>1){
		//说明已经有其他进程打开oldname文件了，不能修改文件名
		goto label_clear_inode;
	}
	//判断newname目录是否正确，并且newname不存在
	memcpy((void*)va2la(TASK_FS, newname), (void*)va2la(src, p_msg->NEWNAME), newname_len);
	newname[newname_len] = 0;
	inode_idx = search_file(newname, newfilename, &new_dir_inode);
	if(inode_idx != INVALID_INODE){
		//newname目录错误 inode_idx == INVALID_PATH
		//或者 已经存在同名的文件 inode_idx >0
		goto label_clear_inode;
	}
	//HOOK POINT
	//考虑挂载fat12文件系统的情况下的rename比较复杂
	//1. 同一个文件系统下，可以简单的修改目录项
	//2. 不同文件系统下，需要移动文件数据
	if(old_pinode->i_dev == new_dir_inode->i_dev){
		// 在相同的文件系统内
		// 修改文件名(移动树形目录的子树节点)
		// 由于TASK_FS一次只处理其他进程的单个消息，所以以下操作是原子的
		//修改的时候，也要修改pinode的parent链
		// 先在newname所在目录下创建目录项
		new_dir_entry(new_dir_inode, old_pinode->i_num, newfilename);
		// 再将oldname所在目录的目录项删除
		rm_dir_entry(old_dir_inode, old_pinode->i_num);
		//修改parent inode指针, 否则下面CLEAR_INODE的时候会出错
		new_pinode = old_pinode;
		new_pinode->parent = new_dir_inode;
		old_pinode = 0;
		ret = 0;
	} else {
		//在不同的文件系统内
	}
label_clear_inode:
	CLEAR_INODE(old_dir_inode, old_pinode);
	CLEAR_INODE(new_dir_inode, new_pinode);
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
	int inode_idx = 0;
	char filename[MAX_PATH];
	char pathname[MAX_PATH];
	struct inode *target_pinode = 0, *target_dir_inode = 0, *source_pinode = 0, *source_dir_inode = 0;
	int path_name_len = p_msg->NAME_LEN;
	assert(path_name_len < MAX_PATH);
	char devname[MAX_PATH];
	int dev_name_len = p_msg->DEVNAME_LEN;	
	assert(dev_name_len < MAX_PATH);
	int src = p_msg->source;
	memcpy((void*)va2la(TASK_FS, pathname), (void*)va2la(src, p_msg->PATHNAME), path_name_len);
	pathname[path_name_len] = 0;
	//step1  判断挂载目录是否是目录
	inode_idx = search_file(pathname, filename, &target_dir_inode);
	//if(inode_idx == INVALID_INODE || target_dir_inode == 0){
	if(inode_idx == INVALID_INODE || inode_idx == INVALID_PATH){
		//target的目录不对
		goto label_fail;
	}
	target_pinode = get_inode(target_dir_inode, inode_idx);
	if(!target_pinode){
		//target 文件不存在
		goto label_fail;
	}
	if(target_pinode->i_mode != I_DIRECTORY){
		//不是目录文件
		goto label_fail;
	}
	if(target_pinode->i_cnt > 1){
		//该目录下有文件被打开或该目录是某进程的运行时目录
		goto label_fail;
	}
	//step2 判断是否已经挂载
	//TODO 
	memcpy((void*)va2la(TASK_FS, devname), (void*)va2la(src, p_msg->DEVNAME), dev_name_len);
	devname[dev_name_len] = 0;
	//step3 判断是否是设备文件
	inode_idx = search_file(devname, filename, &source_dir_inode);
	//if(inode_idx == INVALID_INODE || source_dir_inode == 0){
	if(inode_idx == INVALID_INODE || inode_idx == INVALID_PATH){
		//dev 路径无效
		goto label_fail;
	}
	source_pinode = get_inode(source_dir_inode, inode_idx);
	if(!source_pinode){
		//无效dev文件
		goto label_fail;
	}
	if(source_pinode->i_mode != I_BLOCK_SPECIAL){
		//不是设备文件
		goto label_fail;
	}
	
	//validate
	//首先打开设备文件
	struct message driver_msg;
	reset_msg(&driver_msg);
	driver_msg.type = DEV_OPEN;
	int dev = source_pinode->i_start_sect;
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
	target_pinode->i_dev = dev; //下次再打开挂在目录下的文件的时候，文件的dev就会因为从父目录中获取，从而改编成挂在的dev了
	//挂载同时还需要读取软盘BPB
	init_fat12_bpb(dev);
	//target_pinode的其他属性也要修改，如所在设备开始扇区号
	
	//TODO 获取目录所在扇区及大小
	//FAT12软盘中存储的结构与硬盘中的结构不同，所以需要在inode_table中开辟一个inode节点，虚拟的表示floppy中的文件
	//为了简化起见，fat12只允许一级目录，根目录下的目录将会被忽略
	struct BPB * bpb_ptr = FAT12_BPB_PTR(dev);
	target_pinode->i_start_sect = bpb_ptr->rsvd_sec_cnt + bpb_ptr->hidd_sec + bpb_ptr->num_fats * (bpb_ptr->fat_sz16>0 ? bpb_ptr->fat_sz16 : bpb_ptr->tot_sec32);//【BPB结构中的(rsvd_sec_cnt + hidd_sec + num_fats* (fat_sz16>0?fat_sz16:tot_sec32))】 //root目录开始扇区
	//target_pinode->i_sects_count = bpb_ptr->root_ent_cnt * sizeof(struct fat12_dir_entry)/bpb_ptr->bytes_per_sec; //【root_ent_cnt*sizeof(struct RootEntry)/bytes_per_sec】 //根目录最大文件数*每个目录项所占字节数/每个扇区的字节数=占用扇区数
	target_pinode->i_size = bpb_ptr->root_ent_cnt * sizeof(struct fat12_dir_entry); //【root_ent_cnt*sizeof(struct RootEntry)】
	target_pinode->i_sects_count = target_pinode->i_size / bpb_ptr->bytes_per_sec;
	target_pinode->i_num = ROOT_INODE;		//【说明】本来预计floppy文件的inode值就设置成第一个簇号
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
	CLEAR_INODE(source_dir_inode, source_pinode);
	return 0;
label_fail:
	CLEAR_INODE(target_dir_inode, target_pinode);
	CLEAR_INODE(source_dir_inode, source_pinode);
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
	char pathname[MAX_PATH];
	int name_len = p_msg->NAME_LEN;
	assert(name_len < MAX_PATH);
	int src = p_msg->source;
	memcpy((void*)va2la(TASK_FS, pathname), (void*)va2la(src, p_msg->PATHNAME), name_len);
	pathname[name_len] = 0;
	//验证
	//step1
	struct inode *dir_inode = 0, *pinode = 0;
	char filename[MAX_PATH];
	int inode_idx = search_file(pathname, filename, &dir_inode);
	//if(inode_idx == INVALID_INODE || dir_inode == 0){
	if(inode_idx == INVALID_INODE || inode_idx == INVALID_PATH){
		//路径无效
		goto label_fail;
	}
	pinode = get_inode(dir_inode, inode_idx);
	if(!pinode){
		//无效文件
		goto label_fail;
	}
	if(pinode->i_mode != I_DIRECTORY){
		//卸载不是目录
		goto label_fail;
	}
	//TODO 判断是否被挂载过
	if(pinode->i_dev == ROOT_DEV){
		//目录dev是ROOT_DEV ,说明没有被挂在过
		goto label_fail;
	}
	//关闭设备文件
	struct message driver_msg;
	reset_msg(&driver_msg);
	driver_msg.type = DEV_CLOSE;
	int dev = pin->i_start_sect; //dev设备号存储在磁盘的i_start_sect区域
	driver_msg.DEVICE=MINOR(dev);
	//dd_map[1] = TASK_FLOPPY
	//assert(MAJOR(dev)==1); //可能有多个块设备
	assert(dd_map[MAJOR(dev)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(dev)].driver_pid, &driver_msg);
	assert(driver_msg.type==SYSCALL_RET);
	//设备关闭成功
	pinode->i_dev = ROOT_DEV; //这步操作不是不要的，因为下面就要关闭pinode了，下次再打开的目录下的文件时候，就会从新从/打开，从而i_dev会变成ROOT_DEV
	//其他属性由于没有记录，所以不能还原了，只能依靠CLEAR_INODE之后，下次再打开的时候从硬盘还原
	clear_fat12_bpb(dev);
	//清理pinode
	CLEAR_INODE(dir_inode, pinode);
	return 0;
label_fail:
	CLEAR_INODE(dir_inode, pinode);
	return -1;
}
