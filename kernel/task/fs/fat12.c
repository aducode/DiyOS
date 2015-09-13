/******************************************************/
/*                   fat12文件系统                    */
/******************************************************/
//主要供fs.c使用，用来将fat12格式的文件系统，适配到自己的文件系统
#include "floppy.h"
#include "fat12.h"
#include "fs.h"
#include "assert.h"
#include "string.h"
#include "proc.h"

/**
 * @function init_fat12_bpb
 * @brief 根据设备号获取软盘的BPB
 *			有缓存功能
 * @param dev
 */
static void init_fat12_fs(int dev);

/**
 * @function clear_fat12_bpb
 * @brief 清空bpb缓存
 * @param  dev 设备号
 * @return
 */
static void clear_fat12_bpb(int dev);
/**
 * @function dump_bpb
 * @brief dump出bpb的值
 */
static void dump_bpb(struct BPB * bpb_ptr);
/**
 * @function dump_entry
 * @brief dump出entry的值
 */
static void dump_entry(struct fat12_dir_entry * entry_ptr);

/**
 * @function get_inode_fat12
 * @breif adapter get_inode function in FAT12 File System
 * @param parent 父目录inode指针
 * @param inode_idx inode idx(fat12 file system下，除了根目录(1)以外，其他值为文件开始簇号)
 * @return inode ptr
 */
static struct inode * get_inode_fat12(struct inode *parent, int inode_idx);

/**
 * @function get_inode_idx_from_dir_fat12
 * @brief 从目录项中得到inode idx
 *			inode idx在fat12中就是文件起始簇号
 * @param parent 父目录的inode指针
 * @param filename 文件名
 * @return 返回inode idx
 */
static int get_inode_idx_from_dir_fat12(struct inode *parent,  const char * filename);

/**
 * @function do_rdwt_fat12
 * @breif 适配rootfs的do_rdwt
 * @param pinode  inode指针
 * @param buf 输出的缓冲
 * @param pos 当前文件偏移
 * @param len读写的字节数
 * @param src_pid 输出的进程ID
 * @return 读写的字节数
 */
static int do_rdwt_fat12(int io_type, struct inode *pinode, void * buf, int pos,  int len);

/**
 * @function unlink_file_fat12
 * @brief 适配rootfs的unlink_file
 * @param pinode
 * @return 0 if success
 */
static int unlink_file_fat12(struct inode *pinode);

/**
 * @function create_file_fat12
 * @brief 创建目录
 * @param parent
 * @param filename
 * @param type
 * @return inode ptr
 */
static struct inode * create_file_fat12(struct inode *parent, const char * filename, int flags);

/**
 * @function create_directory_fat12
 * @brief 创建目录
 * @param parent
 * @param filename
 * @param type
 * @return inode ptr
 */
static struct inode * create_directory_fat12(struct inode *parent, const char * filename, int flags);

/**
 * @function is_dir_empty
 * @brief 判读pinode指向的目录是否是空目录
 * @param pinode
 * @return 0 false 1 true
 */
static int is_dir_emtpy(struct inode *pinode);

/**
 * @function new_dir_entry_fat12
 * @brief fat12下的new dir entry
 * @param dir_inode
 * @param fst_clus 文件第一个簇号
 * @param filename 文件名
 */
static void new_dir_entry_fat12(struct inode *dir_inode, int fst_clus, const char *filename);

/**
 * @function rm_dir_entry_fat12
 * @brief fat12下的rm dir entry
 * @param dir_inode
 * @param fst_clus
 */
static void rm_dir_entry_fat12(struct inode *dir_inode, int fst_clus);

/**
 * @function sync_inode_fat12
 * @brief 持久化inode
 * @param pinode
 */
static void sync_inode_fat12(struct inode *pinode);
/**
 * @function fmt_fat12
 * @brief 格式化fat12
 */
static void fmt_fat12();

/**
 * @function mount_for_fat12
 * @brief 挂载fat12
 * @param pinode 挂载点
 * @param dev  挂载设备dev号
 */
static void mount_for_fat12(struct inode *pinode, int dev);

//卸载
static void unmount_for_fat12(struct inode * pinode, int newdev);

struct abstract_file_system fat12 = {
	FLOPPYA_DEV,					//dev
	0,								//is_root
	init_fat12_fs,						//init_fs_func
	get_inode_fat12,				//get_inode_func
	//sync_inode_fat12,				//sync_inode_func
	get_inode_idx_from_dir_fat12,	//get_inode_num_from_dir_func
	do_rdwt_fat12,					//rdwt_func
	create_file_fat12,				//create_file_func
	0,								//create_special_file_func
	create_directory_fat12,			//create_directory_func
	unlink_file_fat12,				//unlink_file_func
	is_dir_emtpy,					//is_dir_emtpy_func
	new_dir_entry_fat12,			//new_dir_entry_func
	rm_dir_entry_fat12,				//rm_dir_entry_func
	mount_for_fat12,				//mount_func
	unmount_for_fat12,				//unmount_func
	fmt_fat12						//format_func
}
/**
 * 缓存软盘BPB
 * 一般只允许fs.c和fat12.c使用
 */
struct BPB FAT12_BPB[MAX_FAT12_NUM];

void init_fat12(){
	//唯一要做的就是初始化全局BPB表
	memset(FAT12_BPB, 0, sizeof(FAT12_BPB));
}

void init_fat12_fs(int dev)
{
	int idx = MINOR(dev);
	assert(idx == 0||idx==1); //floppya floppyb
	if(FAT12_BPB[idx].i_cnt == 0){
		//TODO 读取软盘BPB
		
		//i_cnt会被软盘数据覆盖
		//需要重新置为1
		FAT12_BPB[idx].i_cnt = 1;
	} else {
		//说明已经挂载过了
		FAT12_BPB[idx].i_cnt++;
	}
}

void clear_fat12_bpb(int dev)
{
	int idx = MINOR(dev);
	assert(idx == 0||idx == 1);
	if(FAT12_BPB[idx].i_cnt > 0){
		FAT12_BPB[idx].i_cnt--;
	}
}

struct inode * get_inode_fat12(struct *parent, int inode_idx)
{
	if(inode_idx==0){//0号inode没有使用
		return 0;
	}
	//step1 先判读inode table中是否已经存在
	struct inode * p;
	struct inode * q = 0;
	for(p= inode_table ; p<inode_table + MAX_INODE_COUNT; p++){
		if(p->i_cnt){
			if((p->i_dev == parent->i_dev) && (p->i_num == inode_idx) && (p->i_parent == parent)){
				p->i_cnt++;
				return p;
			}
		} else {
			//a free slot
			if(!q){
				q = p;
			}
		}
	}
	if(!q){
		panic("the inode able is full");
	}
	//inode table之前没有缓存inode， 需要初始化这个inode
	//fat12的文件大小信息保存在目录项里面，所以还需要先从软盘中读取目录信息
	//TODO step 2 读取目录信息到BUF
	//
	//struct fat12_dir_entry * dir_entry_ptr;
	//loop
	//if(dir_entry_ptr->fst_clus == inode_idx); //不同的文件对应的开始簇号是唯一的
	//int file_size = dir_entry_ptr->file_size; //得到文件大小
	//q->i_mode = is_dir(dir_entry)? I_DIRECTORY:I_REGULAR  //软盘中的，暂时使用这两种吧
	return q;
}

int get_inode_idx_from_dir_fat12(struct inode *parent,  const char * filename)
{
	assert(parent != 0);
	assert(filename[0] != 0);
	//读取目录项
	//loop
	//	if(dir_entry->name)
	//return dir_entry->fst_clus;
	return INVALID_INODE;
}

int do_rdwt_fat12(int io_type, struct inode *pinode, void * buf, int pos, int len)
{
	int bytes_rdwt = 0; //已读写字节数
	struct inode *dir_inode = pinode->i_parent; //得到父目录的inode
	//TODO从fat12中得到目录项
	//loop 遍历目录项，得到匹配文件，进而得到文件开始簇号 和 fat12表项所指向的下一个簇号
	//根据pos len 读写数据
	//同步文件变化
	return 0;
	
}

int unlink_file_fat12(struct inode *pinode)
{
	struct inode *dir_inode = pinode->i_parent;
	//1.清空目录项
	//2.清空fat表链
	rm_dir_entry_fat12(pinode, pinode->i_num);
	return 0;
}


struct inode * create_file_fat12(struct inode *parent, const char * filename, int type, int flags)
{
	int fst_clus;
	//找到空的簇号，作为目录第一个簇号
	//创建新的目录项
	struct inode *pinode = get_inode_fat12(parent, fst_clus);
	return pinode;
}

struct inode * create_directory_fat12(struct inode *parent, const char * filename, int type, int flags)
{
	int fst_clus;
	//找到空的簇号，作为目录第一个簇号
	//创建新的目录项
	struct inode *pinode = get_inode_fat12(parent, fst_clus);
	return pinode;
}

int is_dir_emtpy(struct inode *pinode)
{
	struct inode *dir_inode = pinode->i_parent;
	//fat12 目录文件的size字段为0，没办法简单的经过计算得到目录项个数
	// 读取目录项，遍历判断是否是空目录
	// 是空目录返回1
	// 否则返回0
	return 0;
}


void new_dir_entry_fat12(struct inode *dir_inode, int fst_clus, const char *filename)
{
	//TODO fat12 修改目录项
	struct inode *pinode = get_inode_fat12(dir_inode, fst_clus);
}

void rm_dir_entry_fat12(struct inode *dir_inode, int fst_clus)
{
	//TODO fat12 删除目录项
}

void sync_inode_fat12(struct inode *pinode)
{
	//fat12的文件系统里没有物理意义上的inode部分，所以不需要sync
	//DO NOTHING
}

void mount_for_fat12(struct inode *pinode, int dev)
{
	//在fat12文件系统上挂载
	
}

void unmount_for_fat12(struct inode * pinode, int newdev)
{
	//fat12文件系统上卸载
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
	pinode->i_dev = newdev; //这步操作不是不要的，因为下面就要关闭pinode了，下次再打开的目录下的文件时候，就会从新从/打开，从而i_dev会变成ROOT_DEV
	//其他属性由于没有记录，所以不能还原了，只能依靠CLEAR_INODE之后，下次再打开的时候从硬盘还原
	clear_fat12_bpb(dev);
}

void fmt_fat12()
{
	//do nothing
}
void dump_bpb(struct BPB *bpb_ptr)
{
	assert(bpb_ptr != 0);
	printk("BPB:\n");
	printk("\tBPB_BytsPerSec:%d\n", bpb_ptr->bytes_per_sec);
	printk("\tBPB_SecPerClus:%d\n", bpb_ptr->sec_per_clus);
	printk("\tBPB_RsvdSecCnt:%d\n", bpb_ptr->rsvd_sec_cnt);
	printk("\tBPB_NumFATs:%d\n", bpb_ptr->num_fats);
	printk("\tBPB_RootEntCnt:%d\n", bpb_ptr->root_ent_cnt);
	printk("\tBPB_TotSec16:%d\n", bpb_ptr->tot_sec16);
	printk("\tBPB_Media:%d\n", bpb_ptr->media);
	printk("\tBPB_FATSz16:%d\n", bpb_ptr->num_fats);
	printk("\tBPB_SecPerTrk:%d\n", bpb_ptr->sec_per_trk);
	printk("\tBPB_NumHeads:%d\n", bpb_ptr->num_heads);
	printk("\tBPB_HiddSec:%d\n", bpb_ptr->hidd_sec);
	printk("\tBPB_TotSec32:%d\n", bpb_ptr->tot_sec32);
}

void dump_entry(struct fat12_dir_entry *entry_ptr)
{
	assert(entry_ptr != 0);
	char filename[13]; //全名
	int ch;
	int i = 0, j=0;
	while ((ch = (int)*(entry_ptr->name + i)) != 0 && ch != 32){
		filename[i++] = ch;
	}
	if (entry_ptr->suffix[0] != '\0' && entry_ptr->suffix[0] != ' '){
		//没有后缀名
		filename[i++] = '.';
		while ((ch = (int)*(entry_ptr->suffix + j++)) != 0 && ch != 32){
			filename[i++] = ch;
		}
	}
	filename[i] = 0;
	printk("Entry:\n");
	printk("\tname:%s\n", filename);
	printk("\tattr:0x%x\n", entry_ptr->attr);
	printk("\twrt_time:%d\n", entry_ptr->wrt_time);
	printk("\twrt_date:%d\n", entry_ptr->wrt_date);
	printk("\tfst_clus:%d\n", entry_ptr->fst_clus);
	printk("\tfile_size:%d\n", entry_ptr->file_size);
	if (is_dir(entry_ptr)) {
		printk("\tdirectory\n");
	}
	else {
		printk("\tfile\n");
	}
}
