/******************************************************/
/*                   fat12文件系统                    */
/******************************************************/
//主要供fs.c使用，用来将fat12格式的文件系统，适配到自己的文件系统
#include "fat12.h"
#include "assert.h"
void dump_bpb(struct BPB *bpb_ptr)
{
	assert(bpb_ptr != 0);
	printf("BPB:\n");
	printf("\tBPB_BytsPerSec:%d\n", bpb_ptr->bytes_per_sec);
	printf("\tBPB_SecPerClus:%d\n", bpb_ptr->sec_per_clus);
	printf("\tBPB_RsvdSecCnt:%d\n", bpb_ptr->rsvd_sec_cnt);
	printf("\tBPB_NumFATs:%d\n", bpb_ptr->num_fats);
	printf("\tBPB_RootEntCnt:%d\n", bpb_ptr->root_ent_cnt);
	printf("\tBPB_TotSec16:%d\n", bpb_ptr->tot_sec16);
	printf("\tBPB_Media:%d\n", bpb_ptr->media);
	printf("\tBPB_FATSz16:%d\n", bpb_ptr->num_fats);
	printf("\tBPB_SecPerTrk:%d\n", bpb_ptr->sec_per_trk);
	printf("\tBPB_NumHeads:%d\n", bpb_ptr->num_heads);
	printf("\tBPB_HiddSec:%d\n", bpb_ptr->hidd_sec);
	printf("\tBPB_TotSec32:%d\n", bpb_ptr->tot_sec32);
}