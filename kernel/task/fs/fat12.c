/******************************************************/
/*                   fat12文件系统                    */
/******************************************************/
//主要供fs.c使用，用来将fat12格式的文件系统，适配到自己的文件系统
#include "hd.h"
#include "fat12.h"
#include "assert.h"
#include "string.h"

/**
 * 缓存软盘BPB
 * 一般只允许fs.c和fat12.c使用
 */
struct BPB FAT12_BPB[MAX_FAT12_NUM];

void init_fat12(int dev)
{
	//唯一要做的就是初始化全局BPB表
	memset(FAT12_BPB, 0, sizeof(FAT12_BPB));
}

void init_fat12_bpb(int dev)
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