/******************************************************/
/*                   fat12文件系统                    */
/******************************************************/
//主要供fs.c使用，用来将fat12格式的文件系统，适配到自己的文件系统
#include "fat12.h"
#include "assert.h"

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