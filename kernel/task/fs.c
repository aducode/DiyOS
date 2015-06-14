/******************************************************/
/*                 操作系统的文件系统                 */
/******************************************************/
#include "type.h"
#include "syscall.h"
#include "global.h"
#include "assert.h"
#include "hd.h"
#include "fs.h"
#include "stdio.h"
#include "proc.h"

#include "klib.h"
static void init_fs();
static void init_super_block(struct super_block * p_sb);
static void init_inode_bitmap();
static void init_sect_bitmap(struct super_block * p_sb);
static void init_inode_array(struct super_block * p_sb);
static void init_data_blocks(struct super_block * p_sb);
static void fmtfs();
static void mkfs();
static void read_super_block(int dev);
static struct super_block * get_super_block(int dev);
static struct inode * get_inode(int dev, int inode_idx);
static void put_inode(struct inode *pinode);
static void sync_inode(struct inode *p);
static int do_open(struct message *p_msg);
static int do_close(struct message *p_msg);
static int do_rdwt(struct message *p_msg);
static int do_seek(struct message *p_msg);
static int do_tell(struct message *p_msg);
static int do_unlink(struct message *p_msg);
static int fs_fork(struct message *msg);
static int fs_exit(struct message *msg);
static struct inode * create_file(char *path, int flags);
static int alloc_imap_bit(int dev);
static int alloc_smap_bit(int dev, int sects_count_to_alloc);
static struct inode* new_inode(int dev, int inode_nr, int start_sect);
static void new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename);
static int strip_path(char *filename, const char *pathname, struct inode **ppinode);
static int search_file(char *path);

/**
 * Main Loop 
 */
void task_fs()
{
	//_disp_str("fs...",23,0,COLOR_GREEN);
	//printk("Task FS begins.\n");
/*
	struct message msg;
	msg.type = DEV_OPEN;
	msg.DEVICE = MINOR(ROOT_DEV);		//ROOT_DEV=MAKE_DEV(DEV_HD, MINOR_hd2a) 是文件系统的根分区 我们的次设备号定义 hd2a 就是第2主分区（扩展分区）的第一个逻辑分区，主设备号是DEV_HD 表明设备是硬盘
	//这个设备号的主设备号对应的是消息的目标，根据dd_map可以由主设备号映射到驱动进程的pid，也就是消息的目标
	//次设备号就是msg中的DEVICE
	assert(dd_map[MAJOR(ROOT_DEV)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_pid, &msg);
	printk("close dev\n");
	msg.type=DEV_CLOSE;
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_pid, &msg);
	spin("FS");
*/
	init_fs();
	struct message msg;
	while(1){
//		printk("fs running...\n");
		//wait for other process
		send_recv(RECEIVE, ANY, &msg);
		int src = msg.source;
		switch(msg.type){
			case OPEN:
				//_disp_str("FS handle open", 0, 0, COLOR_GREEN);
				//open 要返回FD
				msg.FD = do_open(&msg);
				break;
			case CLOSE:
				//返回是否成功
				msg.RETVAL = do_close(&msg);
				break;
			case READ:
			case WRITE:
				msg.CNT = do_rdwt(&msg);
				break;
			case SEEK:
				msg.RETVAL = do_seek(&msg);
				break;
			case TELL:
				msg.POSITION = do_tell(&msg);
				break;
			case UNLINK:
				msg.RETVAL = do_unlink(&msg);
				break;
			case RESUME_PROC:
				src = msg.PID; //恢复进程,此时将src变成TTY进程发来的消息中的PID
				break;
			case FORK:
				msg.RETVAL = fs_fork(&msg);
				break;
			case EXIT:
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
	struct message msg;
	msg.type = DEV_OPEN;
	msg.DEVICE = MINOR(ROOT_DEV);
	assert(dd_map[MAJOR(ROOT_DEV)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_pid, &msg);
	sb = (struct super_block*)fsbuf;
	//打开后创建文件系统
	//fmtfs();
	mkfs();
	//load super block of ROOT
	read_super_block(ROOT_DEV);
	sb=get_super_block(ROOT_DEV);
	assert(sb->magic == MAGIC_V1);
	root_inode = get_inode(ROOT_DEV, ROOT_INODE);
	//test
	//这里测试多次也都能获取到root_inode在磁盘中的内容，
	//可是为什么多次调用strip_path，里面的类似内容却不能正常运行？
	//TODO find out
	int ii;
	for(ii=0;ii<3;ii++){
		assert(root_inode->i_size%DIR_ENTRY_SIZE==0);
		int repeat = root_inode->i_size/DIR_ENTRY_SIZE;
		printk("%d/%d=%d\n", root_inode->i_size, DIR_ENTRY_SIZE,repeat);
		struct dir_entry de , *pde;
		READ_SECT(root_inode->i_dev, root_inode->i_start_sect);
		pde=(struct dir_entry*)fsbuf;
		for(i=0;i<repeat;i++,pde++){
			memcpy((void*)&de, (void*)(fsbuf + i * DIR_ENTRY_SIZE), DIR_ENTRY_SIZE);
			printk("[%d]de.inode_idx=%d,de.name=%s\n", ii,de.inode_idx, de.name);
			printk("[%d]pde->inode_idx=%d,pde->name=%s\n",ii, pde->inode_idx, pde->name);
		}
	}
	assert(0);
/*
	printk("root_inode->i_dev:%d,i_start_sect:%d, i_sects_count:%d\n",root_inode->i_dev, root_inode->i_start_sect, root_inode->i_sects_count);
	//memset(fsbuf, 0, SECTOR_SIZE);
	//test READ_SECT & WRITE_SECT
	int repeat,t;
	char tmp [SECTOR_SIZE];
	char tmp2[SECTOR_SIZE];
	for(repeat=0;repeat<8;repeat++){
		//memset(fsbuf, 0, SECTOR_SIZE);
		//for(t=0;t<SECTOR_SIZE-1;t++){
		//	tmp[i]='#';
		//}
		//tmp[100]=0;
		//printk("%s\n", fsbuf);
		sprintf(tmp, "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789%d", repeat);
		//sprintf(tmp, "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012%d", repeat);
		strcpy(fsbuf, tmp);
		WRITE_SECT(root_inode->i_dev, root_inode->i_start_sect);
		READ_SECT(root_inode->i_dev, root_inode->i_start_sect);
		strcpy(tmp2, fsbuf);
		printk("%d:%d---%d\n",strlen(tmp), strlen(tmp2),strcmp(tmp, tmp2));
		READ_SECT(root_inode->i_dev, root_inode->i_start_sect);
		strcpy(tmp2, fsbuf);
		printk("%d:%d+++%d\n",strlen(tmp), strlen(tmp2), strcmp(tmp, tmp2));
	}
	assert(0);
	READ_SECT(root_inode->i_dev, root_inode->i_start_sect);
	printk("\n%s\n", fsbuf);	
	char filename[MAX_PATH];
	struct inode * tinode;
	assert(strip_path(filename, "/dev_tty0", &tinode)!=-1);
	assert(tinode == root_inode);
	assert(strip_path(filename, "/dev_tty1", &tinode)!=-1);
	assert(tinode == root_inode);
	assert(strip_path(filename, "/dev_tty2", &tinode)!=-1);
	assert(tinode == root_inode);
*/
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
	printk("dev size: %d sectors\n", geo.size);
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
 * @function mkfs
 * @brief Make a avaliable DIYOS's FS in the disk. It will
 *	- Write a super block to sector 1.
 *	- Create three special files:dev_tty0, dev_tty1, dev_tty2
 *	- Create the inode map
 *	- Create the sector map
 *	- Create the inodes of the files
 *	- Create '/', the root directory
 * @return
 */
void mkfs()
{
	struct message msg;
	int i, j;
	int bits_per_sect = SECTOR_SIZE * 8; //8 bits per byte
	//get the geometry of ROOTDEV
	struct part_info geo;
	msg.type	= DEV_IOCTL;
	msg.DEVICE	= MINOR(ROOT_DEV);
	msg.REQUEST	= DIOCTL_GET_GEO;
	msg.BUF		= &geo;
	msg.PID		= TASK_FS;
	assert(dd_map[MAJOR(ROOT_DEV)].driver_pid != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_pid, &msg);
	//printk("dev size: 0x%x sectors\n", geo.size);
	//super block
	struct super_block sb;
	sb.magic		= MAGIC_V1;
	sb.inodes_count		= bits_per_sect;//一般情况下就是4096
	//inode_sects_count inode_array占用的扇区数
	//就是inode数*每个inode结构体大小/每个扇区的字节数
	sb.inode_sects_count	= sb.inodes_count * INODE_SIZE/SECTOR_SIZE;
	//全部扇区数
	sb.sects_count		= geo.size;
	//inode 位图占用一个扇区
	sb.imap_sects_count	= 1;
	//sector 位图占用的扇区数
	//总扇区数/每扇区位数+1
	sb.smap_sects_count	= sb.sects_count/bits_per_sect + 1;
	//第一个数据扇区
	//跳过boot sector, super block sector，inode bitmap sector[1], sectors bitmap sectors, inode_array sectors
	sb.first_sect		= 1 + 1 /*boot sector & super block*/+ sb.imap_sects_count + sb.smap_sects_count + sb.inode_sects_count;
	sb.root_inode		= ROOT_INODE;
	sb.inode_size		= INODE_SIZE;
	struct inode x;
	sb.inode_isize_off	= (int)&x.i_size - (int)&x;
	sb.inode_start_off	= (int)&x.i_start_sect - (int)&x;
	sb.dir_ent_size		= DIR_ENTRY_SIZE;
	struct dir_entry de;
	sb.dir_ent_inode_off	= (int)&de.inode_idx - (int)&de;
	sb.dir_ent_fname_off	= (int)&de.name - (int)&de;
	memset(fsbuf, 0x90, SECTOR_SIZE);
	memcpy(fsbuf, &sb, SUPER_BLOCK_SIZE);
	//printk("mkfs\ninodes_count:%d\nsects_count:%d\nimap_sects_count:%d\nsmap_sects_count:%d\nfirst_sect:%d\ninode_sects_count:%d\n",sb.inodes_count,sb.sects_count, sb.imap_sects_count, sb.smap_sects_count, sb.first_sect, sb.inode_sects_count);
	//write the super block
	WRITE_SECT(ROOT_DEV, 1);  //sector 0为boot sector
				  //sector 1super block 写入1
	/*
	printk("devbase:0x%x00, sb:0x%x00, imap:0x%x00, smap:0x%x00 "
		"inodes:0x%x00 1st_sector:0x%x00\n",	
		geo.base * 2,
		(geo.base + 1) * 2,
		(geo.base + 1 + 1) * 2,
		(geo.base + 1 + 1 + sb.imap_sects_count) * 2,
		(geo.base + 1 + 1 + sb.imap_sects_count + sb.smap_sects_count) * 2,
		(geo.base + sb.first_sect) * 2);
	*/
	//inode map
	memset(fsbuf, 0, SECTOR_SIZE);
	for(i=0;i<(CONSOLE_COUNT + 3);i++){
		fsbuf[0] |= 1<<i;
	}
	assert(fsbuf[0] == 0x3F);/* 0011 1111
				  *   || ||||
				  *   || |||`----bit 0: reserved
				  *   || ||`-----bit 1:the first inode, '/'
				  *   || |`------bit 2:/dev_tty0
				  *   || `-------bit 3:/dev_tty1
				  *   |`---------bit 4:/dev_tty2
				  *   `----------bit 5:/cmd.tar
				  */
	WRITE_SECT(ROOT_DEV, 2);//secotr 2为inode sector
	//sector map
	memset(fsbuf, 0, SECTOR_SIZE);
	//DEFAULT_FILE_SECTS_COUNT 每个文件默认占用的扇区数
	//目前是固定值2048
	int nr_sects = DEFAULT_FILE_SECTS_COUNT + 1;
	/*	       ~~~~~~~~~~~~~~~~~~~~~~~|~~~|
	 *                                    |   `---bit 0 is reserved
	 *                                    `-------for '/'
	 */
	//_disp_str("@",0, 0, COLOR_RED);//display
	for(i=0;i<nr_sects/8;i++){
		fsbuf[i] = 0xFF;
	}
	for(j=0;j<nr_sects % 8;j++){
		fsbuf[i] |= (1<<j);
	}
	//_disp_str("@",0, 0, COLOR_RED); //display
	//sector bitmap 对应位置被设置成1
	WRITE_SECT(ROOT_DEV, 2 + sb.imap_sects_count);//imap_sects_count==1
	//_disp_str("@",0, 0, COLOR_RED);	//display
	memset(fsbuf, 0, SECTOR_SIZE);
	//_disp_str("@",0, 0, COLOR_RED); //display
	//设置其余的sector bitmap的扇区为0
	//_disp_str("it will deadlock",0,0, COLOR_RED);
	for(i=1;i<sb.smap_sects_count;i++){
		//TODO fix
		//something wrong here
		//在循环里面打印信息时，程序又能正常运行！
		//_disp_str("@",0,0, COLOR_GREEN);
		WRITE_SECT(ROOT_DEV, 2 + sb.imap_sects_count + i);
	}
	//_disp_str("@",0, 0, COLOR_RED); //do not display
	//sector map for 'cmd.tar'	
	int bit_offset = INSTALL_START_SECT - sb.first_sect + 1;
	int bit_off_in_sect = bit_offset % (SECTOR_SIZE * 8);
	int bit_left = INSTALL_SECTS_COUNT;
	int cur_sect = bit_offset/(SECTOR_SIZE*8);
	//_disp_str("@",0, 0, COLOR_RED); //do not display
	READ_SECT(ROOT_DEV,2+sb.imap_sects_count + cur_sect);
	//_disp_str("@",0, 0, COLOR_RED); //do not display
	while(bit_left){
		int byte_off = bit_off_in_sect/8;
		fsbuf[byte_off] |= 1 << (bit_off_in_sect % 8);
		bit_left --;
		bit_off_in_sect++;
		if(bit_off_in_sect == (SECTOR_SIZE *8)){
			WRITE_SECT(ROOT_DEV, 2 + sb.imap_sects_count+cur_sect);
			cur_sect++;
			READ_SECT(ROOT_DEV, 2 + sb.imap_sects_count + cur_sect);
			bit_off_in_sect = 0;
		}
	}
	WRITE_SECT(ROOT_DEV, 2+sb.imap_sects_count + cur_sect);
	//_disp_str("@",0, 0, COLOR_RED);
	//inodes
	//设置inode array区域
	//inode for '/'
	memset(fsbuf, 0, SECTOR_SIZE);
	struct inode * pi = (struct inode*)fsbuf;
	pi->i_mode = I_DIRECTORY;
	pi->i_size = DIR_ENTRY_SIZE * 5;//5files  '.',dev_tty0, dev_tty1, dev_tty2 cmd.tar
	pi->i_start_sect = sb.first_sect;
	pi->i_sects_count = DEFAULT_FILE_SECTS_COUNT;
	//inode for /dev_tty0~2
	for(i=0;i<CONSOLE_COUNT;i++){
		pi=(struct inode*)(fsbuf + (INODE_SIZE * (i+1)));
		pi->i_mode = I_CHAR_SPECIAL;
		pi->i_size = 0;
		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
		pi->i_sects_count = 0;
	}
	//inode for cmd.tar
	
	//WRITE_SECT(ROOT_DEV, 2+sb.imap_sects_count + sb.smap_sects_count);
	//for 'cmd.tar'
	pi = (struct inode*)(fsbuf+(INODE_SIZE * (CONSOLE_COUNT+1)));
	pi->i_mode = I_REGULAR;
	pi->i_size = INSTALL_SECTS_COUNT * SECTOR_SIZE;
	pi->i_start_sect = INSTALL_START_SECT;
	pi->i_sects_count = INSTALL_SECTS_COUNT;
	WRITE_SECT(ROOT_DEV, 2+sb.imap_sects_count + sb.smap_sects_count);
	//设置真正的文件数据区域
	//'/'
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry *pde = (struct dir_entry*)fsbuf;
	pde->inode_idx = 1;
	strcpy(pde->name, ".");
	//dir entries of "/dev_tty0~2
	for(i=0;i<CONSOLE_COUNT;i++){
		pde++;
		pde->inode_idx = i+2;
		sprintf(pde->name, "dev_tty%d", i);
	}
	(++pde)->inode_idx = CONSOLE_COUNT + 2;
	strcpy(pde->name, "cmd.tar");
	WRITE_SECT(ROOT_DEV, sb.first_sect);
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
	int inode_nr = search_file(pathname);
	struct inode *pin = 0;
	if(flags & O_CREATE){
		//创建文件
		if(inode_nr){
			printk("file exists.\n");
			return -1;
		} else {
			pin = create_file(pathname, flags);
		}
	} else {
		assert(flags & O_RDWT);
		char filename[MAX_PATH];
		struct inode *dir_inode;
		//printk("line:573\n");
		if(strip_path(filename, pathname, &dir_inode) != 0){
			return -1;
		}
		pin = get_inode(dir_inode->i_dev, inode_nr);
	}
	if(pin){
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
			assert(MAJOR(dev) == 4);
			assert(dd_map[MAJOR(dev)].driver_pid != INVALID_DRIVER);
			//printk("FS send message to %d\n", dd_map[MAJOR(dev)].driver_pid);
			send_recv(BOTH, dd_map[MAJOR(dev)].driver_pid, &driver_msg);
		} else if (imode == I_DIRECTORY) {
			assert(pin->i_num == ROOT_INODE);
		} else {
			assert(pin->i_mode == I_REGULAR);
		}
	} else {
		return -1;
	}
	return fd;
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
	} else {
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
	int inode_nr = search_file(pathname);
	if(inode_nr == INVALID_INODE){
		//file not found
		printk("FS::do_unlink():: search_file() return invalid inode: %s\n", pathname);
		return -1;
	}
	char filename[MAX_PATH];
	struct inode* dir_inode;
	//printk("line:821\n");
	if(strip_path(filename, pathname, &dir_inode)!=0){
		return -1;
	}
	struct inode *pin = get_inode(dir_inode->i_dev, inode_nr);
	if(pin->i_mode != I_REGULAR){
		//can only remove regular files
		printk("cannot remove file %s, because it is not a regular file.\n", pathname);
		return -1;
	}
	if(pin->i_cnt>1){
		//thie file was opened
		printk("cannot remove file %s, because pin->i_cnt is %d.\n", pathname, pin->i_cnt);
		return -1;
	}

	struct super_block *sb = get_super_block(pin->i_dev);
	//free the bit in imap
	int byte_idx = inode_nr/8;
	int bit_idx = inode_nr % 8;
	assert(byte_idx<SECTOR_SIZE);//we have only one imap sector
	//read sector 2 (skip bootsect and superblk):
	READ_SECT(pin->i_dev, 2);
	assert(fsbuf[byte_idx%SECTOR_SIZE]&(1<<bit_idx));
	fsbuf[byte_idx % SECTOR_SIZE] &= ~(1<<bit_idx);
	WRITE_SECT(pin->i_dev, 2);
	//free the bits in smap
	bit_idx = pin->i_start_sect - sb->first_sect + 1;
	byte_idx = bit_idx/8;
	int bits_left = pin->i_sects_count;
	int byte_cnt = (bits_left - (8-(bit_idx % 8)))/8; 
	
	//current sector nr
	int s= 2 + sb->imap_sects_count + byte_idx/SECTOR_SIZE;//2:bootsect + superblk
	READ_SECT(pin->i_dev, s);
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
			WRITE_SECT(pin->i_dev, s);
			READ_SECT(pin->i_dev, ++s);
		}
		assert(fsbuf[i]==0xFF);
		fsbuf[i] = 0;
	}

	//clear the last byte
	if(i==SECTOR_SIZE){
		i=0;
		WRITE_SECT(pin->i_dev, s);
		READ_SECT(pin->i_dev, ++s);
	}
	unsigned char mask = ~((unsigned char)(~0)<<bits_left);
	assert((fsbuf[i]&mask)==mask);
	fsbuf[i] &= (~0)<<bits_left;
	WRITE_SECT(pin->i_dev, s);

	//clear the inode itself
	pin->i_mode = 0;
	pin->i_size = 0;
	pin->i_start_sect = 0;
	pin->i_sects_count = 0;
	sync_inode(pin);
	//release slot in inode_table[]
	put_inode(pin);
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
 * @function create_file
 * @brief 创建新的inode，并设置磁盘上的数据
 * @param path 文件路径
 * @param type  I_REGULAR:普通文件 I_DIRECTORY:目录  I_CHAR_SPECIAL:tty
 * @param flags
 * @param start_sect 普通文件/目录 开始扇区号 设备文件 设备号
 * @param sect_count 占用扇区数 （设备文件0）
 * 
 * @return 新inode指针，失败返回0
 */
//struct inode * create_file(char *path, int type,  int flags, int start_sect, int sect_count)
struct inode * create_file(char *path, int flags)
{
	char filename[MAX_PATH];
	struct inode * dir_inode;
	//printk("line:986\n");
	if(strip_path(filename, path, &dir_inode) != 0){
		return 0;
	}
	int inode_nr = alloc_imap_bit(dir_inode->i_dev);
	int free_sect_nr = alloc_smap_bit(dir_inode->i_dev, DEFAULT_FILE_SECTS_COUNT);
	struct inode *newino = new_inode(dir_inode->i_dev, inode_nr, free_sect_nr);
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
	//printk("alloc_smap_bit\n\ndev:%d,sects_count_to_alloc:%d\n",dev, sects_count_to_alloc);
	int i; //sector index
	int j; //byte index
	int k; //bit index
	
	struct super_block *sb = get_super_block(dev);
	//------------dump super_block
	//printk("\ninodes_count:%d\nsects_counts:%d\nimap_sects_count:%d\nsmap_sects_count:%d\nfirst_sect:%d\ninode_sects_count:%d\n",super_block->inodes_count,super_block->sects_count, super_block->imap_sects_count, super_block->smap_sects_count,super_block->first_sect, super_block->inode_sects_count);
	//------------	
	int smap_blk0_nr = 1 + 1 + sb->imap_sects_count;
	//printk("smap_blk0_nr:%d\n", smap_blk0_nr);
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
			//printk("free_sect_nr:%d\n", free_sect_nr);
			WRITE_SECT(dev, smap_blk0_nr + i); //write fsbuf to dev
		}
		if(sects_count_to_alloc == 0){
			break;
		}
	}
	//printk("the sects_count_to_alloc:%d\n", sects_count_to_alloc);
	assert(sects_count_to_alloc == 0);
	return free_sect_nr;
}

/**
 * @function new_inode
 * @brief 在inode_array中分配一个inode并写入内容
 * @param dev home device of the inode
 * @param inode_nr
 *
 * @return ptr of the new inode
 */
struct inode* new_inode(int dev, int inode_nr, int start_sect)
{
	//get from inode array by inode_nr
	struct inode * new_inode = get_inode(dev, inode_nr);
	new_inode->i_mode = I_REGULAR;
	new_inode->i_size = 0;
	new_inode->i_start_sect = start_sect;
	new_inode->i_sects_count = DEFAULT_FILE_SECTS_COUNT;
	
	new_inode->i_dev = dev;
	new_inode->i_cnt = 1;
	new_inode->i_num = inode_nr;
	
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
void new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename)
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
 * @param[out] filename the string for the result
 * @param[in] pathname the full pathname.
 * @param[out] ppinode the ptr of the dir's inode will be stored here.
 *
 * @return Zero if success, otherwise the pathname is not valid.
 */
int strip_path(char *filename, const char *pathname, struct inode **ppinode)
{
	//以下这段代码从hd中读取扇区时，可能会读不出数据
	//why?
	printk("----------------------------\npathname:%s\n", pathname);
	//printk("root_inode->i_dev=%d\n", root_inode->i_dev);
	int i, k,  nr_dir_blks, dir_entry_count;
	struct inode * pinode;
	struct dir_entry * pde;
	const char * s = pathname;
	char *t;
	//一层一层寻找
	if(s==0){
		return -1; //失败
	}
	if(*s != '/'){
		return -1;//必须绝对路径		
	}
	s++;
	*ppinode = root_inode;	//in global.c
	while(*s){
		t = filename;
		while(*s!='/' && *s!=0){
			*t++=*s++;
		}
		*t=0;
		nr_dir_blks = ((*ppinode)->i_size + SECTOR_SIZE)/SECTOR_SIZE;
		pinode = 0;
		for(k=0;k<nr_dir_blks;k++){
			printk("\tdev:%d, start_sect:%d\n", (*ppinode)->i_dev, (*ppinode)->i_start_sect);
			READ_SECT((*ppinode)->i_dev, (*ppinode)->i_start_sect + i);
			pde = (struct dir_entry*)fsbuf;
			printk("%d,%s\n", pde->inode_idx, pde->name);
			assert((*ppinode)->i_size%DIR_ENTRY_SIZE==0);
			dir_entry_count = (*ppinode)->i_size/DIR_ENTRY_SIZE;
		//	printk("\t%s\n", pde->name);
			for(i=0;i<dir_entry_count;i++, pde++){
				//printk("\t\t%s==%s\n", filename, pde->name);
				if(strcmp(filename, pde->name)==0){
					pinode = get_inode((*ppinode)->i_dev, pde->inode_idx); 
				//	printk("pinode:%d\n", pinode);
					goto find_in_dir_end;
				//	break;
				}
			}
		}
find_in_dir_end:
		if(pinode == 0){
			//printk("FAIL", pathname);
			//assert(0);
			return -1; //说明不存在正确的路径
		}
		if(pinode->i_mode!=I_DIRECTORY){
			//不是目录了
			//有两种可能
			//	1. pathname最后一级是文件
			//	2. pathname中间某一级不是目录（错误）
			if(*s!=0){
				printk("?%d",*s==0?1:0);
				assert(0);
				//说明pathname还没结束，错误
				return -1;
			} else {
				//说明可以结束循环
			}
		} else {
			//下一级目录
			*ppinode = pinode;
		}
		
	}
/*
	const char * s = pathname;
	char *t = filename;
	if(s==0){
		return -1;
	}
	if(*s=='/'){
		s++;
	}
	while(*s){
		//check each character
		if(*s=='/'){
			return -1;//invalid '/'
		}
		*t++=*s++;
		//if filename is too long, just truncate it
		if(t-filename >= MAX_FILENAME_LEN){
			break;
		}
	}
	*t=0;
	*ppinode = root_inode;
*/
	assert(*ppinode == root_inode);
	return 0;
}



/**
 * @function search_file
 * @brief Search the file and return the inode_idx
 * @param[in] path the full path of the file to search
 * @return Ptr to the inode of the file if successful, otherwise zero
 * @ses open()
 * @ses do_open()
 */
int search_file(char *path)
{
	int i,j;
	char filename[MAX_PATH];
	memset(filename, 0, MAX_FILENAME_LEN);
	struct inode* dir_inode;
	//printk("line:1293\n");
	if(strip_path(filename, path, &dir_inode)!=0){
		return 0;
	}
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
			if(memcmp(filename, pde->name, MAX_FILENAME_LEN)==0){
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
 * @param dev device
 * @param inode_idx inode号
 *
 * @return The inode ptr requested
 */
struct inode * get_inode(int dev, int inode_idx)
{
	if(inode_idx==0){//0号inode没有使用
		return 0;
	}
	struct inode * p;
	struct inode * q = 0;
	for(p= inode_table ; p<inode_table + MAX_INODE_COUNT; p++){
		if(p->i_cnt){
			//not a free slot
			if((p->i_dev == dev) && (p->i_num == inode_idx)){
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
	pinode->i_cnt --;
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
