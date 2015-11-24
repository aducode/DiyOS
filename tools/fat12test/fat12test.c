#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat12.h"

static unsigned char FSBUF [10240];
static FILE * fat12;
static void get_bpb(struct BPB * bpb_ptr);
static void dump_bpb(struct BPB * bpb_ptr);
static void dump_entry(struct fat12_dir_entry *entry_ptr);
static int validate(const char * name, int size);
static int get_next_clus(struct BPB * bpb_ptr, FILE * fat12, int clus);
static int fat12read(struct BPB * bpb_ptr, FILE * fat12, struct fat12_dir_entry * dir_entry_ptr, char * buf);
int main(int argv, char ** argc)
{
	if(argv <2){
		printf("Usage:\n\tparam:image path\n");
		return 1;
	}
	char * fileName = *(argc+1);
	if((fat12=fopen(fileName, "r"))==NULL){
		printf("open file %s failed!\n", fileName);
	}
	struct BPB bpb;
	get_bpb(&bpb);
	dump_bpb(&bpb);
	//printf("%d, %d\n", ROOT_ENT_BASE(&bpb), sizeof(struct fat12_dir_entry));
	int base = ROOT_ENT_BASE(&bpb);
	int i, bytes;
	struct fat12_dir_entry dir_entry;
	char out[1024];
	for(i=0;i<bpb.root_ent_cnt;i++){
		fseek(fat12, base, SEEK_SET);
		fread(FSBUF, 1, sizeof(dir_entry), fat12);
		base += sizeof(dir_entry);
		dir_entry = *((struct fat12_dir_entry *)FSBUF);	
		if(VALIDATE(&dir_entry)){
			continue;
		}
		printf("0x%x\n", base);
		dump_entry(&dir_entry);
		if(is_dir(&dir_entry)){
			//子目录
			/*
			int current  = dir_entry.fst_clus;
			printf("%d\n", current);
			while(current < 0xFF8){
				if(current== 0xFF7){
					break;
				}
				fseek(fat12, DATA_BASE(&bpb, current), SEEK_SET);
				int bytes = fread(FSBUF, 1, CLUS_BYTES(&bpb), fat12);
				printf("%d\n", bytes);
				int j;
				struct fat12_dir_entry de;
				for(j=0;j<CLUS_BYTES(&bpb)/sizeof(struct fat12_dir_entry);i++){
					de = *((struct fat12_dir_entry *)FSBUF	+ i);
					if(VALIDATE(&de)){
						continue;
					}
					dump_entry(&de);
				}
			}
			*/			
		} else {
			bytes = fat12read(&bpb, fat12, &dir_entry, out);
			if(bytes>1024){
				 bytes = 1024;
			}
			out[bytes] = 0;
			printf("%s", out);
		}
	}
	//printf("%d\n", get_next_clus(&bpb, fat12, 3));
	int fst_clus;
	fclose(fat12);	
	return 0;
}

void get_bpb(struct BPB * bpb_ptr)
{
	int read_bytes = fread(FSBUF, 1, sizeof(struct BPB)+11, fat12);	
	if(read_bytes!=sizeof(struct BPB)+11){
		printf("invalid file");
		return;
	}
	*bpb_ptr = *((struct BPB *)(FSBUF+11));
}

void dump_bpb(struct BPB *bpb_ptr)
{
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

void dump_entry(struct fat12_dir_entry *entry_ptr)
{
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
        printf("Entry:\n");
        printf("\tname:%s\n", filename);
        printf("\tattr:0x%x\n", entry_ptr->attr);
        printf("\twrt_time:%d\n", entry_ptr->wrt_time);
        printf("\twrt_date:%d\n", entry_ptr->wrt_date);
        printf("\tfst_clus:%d\n", entry_ptr->fst_clus);
        printf("\tfile_size:%d\n", entry_ptr->file_size);
        if (is_dir(entry_ptr)) {
                printf("\tdirectory\n");
        }
        else {
                printf("\tfile\n");
        }
}

/**
 * @function validate
 * @brief 判断是否是合文件名，只能是数字字母和空格
 * @param 名称
 * @param size
 * @return 0 合法 1 非法
 */
int validate(const char * name, int size)
{
	int i;
	char ch;
	for(i=0;i<size;i++){
		ch = *(name + i);
		if(!(
			(ch >= 48 && ch <= 57) ||	//空格
			(ch >= 'A' && ch <= 'Z') ||	//字母
			(ch >= 'a' && ch <= 'z') ||	//小写
			(ch == ' ')			//空格
		)|| ch == 0 ){
			return 1;
		}	
	}
	return 0;	
}

int get_next_clus(struct BPB * bpb_ptr, FILE * fat12, int clus)
{
	fseek(fat12, FAT_TAB_BASE(bpb_ptr) + (clus * 3 / 2), SEEK_SET);
	u16 ret;
	fread(&ret, 1, 2, fat12);
	if(clus % 2 == 0){
		return ret << 4;
	} else {
		return ret >> 4;
	}
}

/**
 * @function fat12read
 * @brief 显示文件内容
 */
int fat12read(struct BPB *bpb_ptr, FILE * fat12, struct fat12_dir_entry * dir_entry_ptr, char * buf){
	char * curr = buf;
	int current_clus = dir_entry_ptr->fst_clus;
	int size = dir_entry_ptr->file_size;
	int bytes;
	int ret = 0;
	while(current_clus < 0xFF8){
		if(current_clus == 0xFF7){
			printf("invalid cluster\n");
			return -1;
		}
		fseek(fat12, DATA_BASE(bpb_ptr, current_clus), SEEK_SET);
		bytes = fread(FSBUF, 1, MIN(CLUS_BYTES(bpb_ptr), size), fat12);
		size -= bytes;
		ret += bytes;
		memcpy(curr, FSBUF, bytes);
		curr += bytes;
		current_clus = get_next_clus(bpb_ptr, fat12, current_clus);
	}
	return ret;
}
