/************************************************************/
/*                       硬盘中断                           */
/************************************************************/
#include "type.h"
#include "string.h"
#include "global.h"
#include "proc.h"
#include "syscall.h"
#include "klib.h"
#include "assert.h"
#include "hd.h"
//设备号规则
//系统只支持两块硬盘
//primary master
/***************************************硬盘1****************************************************/
/*					hd0(0)					                */
/************************************************************************************************/
/*         hd1(1)       *//*        hd2(2)        *//*     hd3(3)          *//*    hd4(4)       */
/************************************************************************************************/
/*hd1a(x)....hd1p(x+15)*//*hd2a(x+16)...hd2p(x+31)*//*hd3a(x+32).hd3p(x+47)*//*hd4a(x+48)...hd4p*/
/*                                                                                              */
/************************************************************************************************/

//primary slave
/***************************************硬盘2****************************************************/
/*                                      hd5(5)                                                  */
/************************************************************************************************/
/*         hd6(6)       *//*        hd7(7)        *//*     hd8(8)          *//*    hd9(9)       */
/************************************************************************************************/
/*hd6a(x+64).hd6p(x+79)*//*hd7a(x+80)...hd7p(x+95)*//*hd8a(x+96)......hd8p*//*hd9a...hd9p(x+127)*/
/*                                                                                              */
/************************************************************************************************/

//hd0 为主盘号 0为次设备号
//hd可以知道主设备号是DEV_HD
//次设备号是分区起的名字
//主设备号用来选择驱动程序（global.c中有映射关系）


//只有这里用到的宏
/**
 * dev次设备号
 */
//首先判断设备号是不是小于MAX_PRIM（最大主分区：9 [0,5表示整个硬盘],[1-4,主硬盘的4个主分区],[5-9 另一块硬盘的4个主分区]）
//如果小，说明是主分区，那么驱动号就是 dev/PRIM_PER_DRIVE[5] 结果只能是0或者1
//否则，说明是逻辑分区，那么驱动号是 (dev-MINOR_hd1a[0x10])/SUB_PER_DRIVE[64] 结果也只可能是0 或者 1
//即设备驱动号是0或者1
#define DRV_OF_DEV(dev) (dev<=MAX_PRIM?\
                        dev/PRIM_PER_DRIVE:\
                        (dev-MINOR_hd1a) / SUB_PER_DRIVE)
static u8 hd_status;
static u8 hdbuf[SECTOR_SIZE * 2];

static void hd_cmd_out(struct hd_cmd *cmd);
static void hd_identify(int drive);


static void print_identify_info(u16* hdinfo);

//
static struct hd_info hd_info[1]; //只有一个硬盘设备
//
static void partition(int device, int style);
static void get_part_table(int drive, int sect_nr, struct part_ent * entry);

//
static void print_hdinfo (struct hd_info * hdi);
static u8 hd_status;
static void hd_handler(int irq_no);
static void hd_cmd_out(struct hd_cmd * cmd);
/*************************** PUBLIC *************************************************/
void init_hd()
{
	int i;
	assert(bda.hd_num>0);
        _disable_irq(AT_WINI_IRQ);
        irq_handler_table[AT_WINI_IRQ] = hd_handler;
        _enable_irq(CASCADE_IRQ);
        _enable_irq(AT_WINI_IRQ);
	for(i=0;i<(sizeof(hd_info)/sizeof(hd_info[0]));i++){
		memset(&hd_info[i],0,sizeof(hd_info[0]));
	}
	hd_info[0].open_cnt = 0;
}

/**
 * 打开设备
 * @param device 设备号（主设备号和次设备号）
 */
void hd_open(int device)
{
	//首先根据设备号获得驱动号
	//如果有需要，可以根据驱动号在global.c中的dd_map中根据驱动号获得驱动服务pid
	int drive = DRV_OF_DEV(device);
	assert(drive == 0);//目前只设置了一块硬盘，所以这个结果肯定是0
		
	hd_identify(drive);
	if(hd_info[drive].open_cnt++ ==0){
		//保证只执行一次
		partition(drive * (PART_PER_DRIVE +1 ), P_PRIMARY);
		//打印信息
		//print_hdinfo(&hd_info[drive]);
	}
	//print_hdinfo(&hd_info[drive]);
}

/**
 * 关闭设比
 * @param device 次设备号
 */
void hd_close(int device)
{
	//获得是哪个硬盘设备 这里只有一个硬盘所以一定是0
	int drive = DRV_OF_DEV(device);
	assert(drive == 0); //only one drive
	hd_info[drive].open_cnt--;
}

/**
 * 读写
 */
void hd_rdwt(struct message *msg)
{
	//根据次设备号获取操作哪一个具体的硬盘
	int drive = DRV_OF_DEV(msg->DEVICE);
	u64 pos = msg->POSITION;
	assert((pos >> SECTOR_SIZE_SHIFT) <(1<<31));
	//we only allow to R/W from a SECTOR boundary;
	assert((pos & 0x1FF) == 0);
	//sector 号 由偏移地址除以sector大小
	u32 sect_nr = (u32)(pos>>SECTOR_SIZE_SHIFT); //pos/SECTOR_SIZE
	int logidx = (msg->DEVICE - MINOR_hd1a) % SUB_PER_DRIVE; //逻辑分区的次设备号
	sect_nr += msg->DEVICE<MAX_PRIM?			//根据DEVICE次设备号是否小于5来判断是一个主分区（扩展分区）还是逻辑分区
		hd_info[drive].primary[msg->DEVICE].base:	//如果是主分区，那么根据主分区表
		hd_info[drive].logical[logidx].base;		//如果是逻辑分区，那么根据逻辑分区表
	struct hd_cmd cmd;
	cmd.features = 0;
	cmd.count = (msg->CNT + SECTOR_SIZE - 1)/SECTOR_SIZE;
	cmd.lba_low = sect_nr & 0xFF;
	cmd.lba_mid = (sect_nr >> 8)&0xFF;
	cmd.lba_high = (sect_nr >> 16) & 0xFF;
	cmd.device = MAKE_DEVICE_REG(1, drive,(sect_nr>>24) & 0xF);
	cmd.command = (msg->type == DEV_READ) ? ATA_READ:ATA_WRITE;
	hd_cmd_out(&cmd);
	int bytes_left = msg->CNT;
	void *la = (void*)va2la(msg->PID, msg->BUF);
	while(bytes_left){
		int bytes = min(SECTOR_SIZE, bytes_left); //以SECTOR为单位读写，如果要求的size大于SECTOR_SIZE那么分多次
		if(msg->type == DEV_READ){//读
			interrupt_wait(); //阻塞，数据准备好时，会发生IO中断，解除阻塞
			_port_read(REG_DATA, hdbuf, SECTOR_SIZE);//开始读
			memcpy(la, (void*)va2la(TASK_HD, hdbuf), bytes);
		} else {//写
			if(!waitfor(REG_STATUS, STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT)){
				panic("hd writing error.");
			}
			_port_write(REG_DATA, la, bytes);
			interrupt_wait(); //写完之后再进行下一次写
		}
		bytes_left -= SECTOR_SIZE;
		la += SECTOR_SIZE;
	}
	
}

/**
 *控制
 */
void hd_ioctl(struct message *msg)
{
	int device = msg->DEVICE;
	int drive = DRV_OF_DEV(device);
	assert(drive==0);
	struct hd_info *hdi = &hd_info[drive];
	
	if(msg->REQUEST == DIOCTL_GET_GEO){
		void *dst = va2la(msg->PID, msg->BUF);
		void *src = va2la(TASK_HD, device < MAX_PRIM?&hdi->primary[device] : &hdi->logical[(device - MINOR_hd1a) % SUB_PER_DRIVE]);
		memcpy(dst, src, sizeof(struct part_info));
	} else {
		assert(0);
	}
}
/********************** PRIVATE *************************************************/
void hd_handler(int irq_no)
{
	hd_status = _in_byte(REG_STATUS);
	inform_int(TASK_HD);	
}

/**
 * <Ring 1> Print disk info.
 * 
 * @param hdi  Ptr to struct hd_info.
 */
void print_hdinfo(struct hd_info * hdi)
{
	int i;
	for (i = 0; i < PART_PER_DRIVE + 1; i++) {
		printk("%sPART_%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		       i == 0 ? " " : "     ",
		       i,
		       hdi->primary[i].base,
		       hdi->primary[i].base,
		       hdi->primary[i].size,
		       hdi->primary[i].size);
	}
	for (i = 0; i < SUB_PER_DRIVE; i++) {
		if (hdi->logical[i].size == 0)
			continue;
		printk("         "
		       "%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		       i,
		       hdi->logical[i].base,
		       hdi->logical[i].base,
		       hdi->logical[i].size,
		       hdi->logical[i].size);
	}
}

/**
 * This routine is called when a device is opened. It reads the partition table(s) and fills the hd_info struct.
 *
 * @param device Device idx
 * @param style P_PRIMARY or P_EXTENED
 */
void partition(int device, int style)
{
	int i;
	int drive = DRV_OF_DEV(device);
	struct hd_info *hdi = &hd_info[drive];
	struct part_ent part_tbl[SUB_PER_DRIVE];
	if(style == P_PRIMARY){
		//主分区
		//获取硬盘中保存的分区表
		get_part_table(drive, drive, part_tbl);
		int nr_prim_parts = 0;
		for(i=0;i<PART_PER_DRIVE;i++){ //0~3
			if(part_tbl[i].sys_id == NO_PART) continue;
			nr_prim_parts++;
			int dev_nr = i + 1; //1~4
			hdi->primary[dev_nr].base = part_tbl[i].start_sect;
			hdi->primary[dev_nr].size = part_tbl[i].nr_sects;
			
			if(part_tbl[i].sys_id == EXT_PART){ //extened
				partition(device + dev_nr, P_EXTENDED);//递归查看扩展分区信息
			}
		}
		assert(nr_prim_parts != 0);
	} else if (style == P_EXTENDED){
		int j = device % PRIM_PER_DRIVE;
		int ext_start_sect = hdi->primary[j].base;
		int s = ext_start_sect;
		int nr_1st_sub = (j-1)*SUB_PER_PART; //0/16/32/48
		for(i=0;i<SUB_PER_PART;i++){
			int dev_nr = nr_1st_sub + i; //0~15 / 16~31 / 32~47 /48~63
			get_part_table(drive, s, part_tbl);
			
			hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
			hdi->logical[dev_nr].size = part_tbl[0].nr_sects;
			
			s = ext_start_sect + part_tbl[1].start_sect;
			
			if(part_tbl[1].sys_id == NO_PART){
				break;
			}
		}
	} else {
		assert(0);
	}
}

/**
 * Get a partition table of a drive
 * @param drive Drive number (0 for the 1st disk, 1 for the 2nd, ...)
 * @param The sector at which the partition table is lcated.
 * @param entry Ptr to part_ent struct.
 */
void get_part_table(int drive, int sect_nr, struct part_ent * entry)
{
	struct hd_cmd cmd;
	cmd.features = 0;
	cmd.count = 1;
	cmd.lba_low = sect_nr & 0xFF;
	cmd.lba_mid = (sect_nr >> 8) & 0xFF;
	cmd.lba_high = (sect_nr >> 16) & 0xFF;
	cmd.device = MAKE_DEVICE_REG(1,//LBA mode
			drive,
			(sect_nr>>24)& 0xF);
	cmd.command = ATA_READ;
	hd_cmd_out(&cmd);
	interrupt_wait();
	_port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	memcpy(entry,hdbuf + PARTITION_TABLE_OFFSET, sizeof(struct part_ent) * PART_PER_DRIVE);
}

void hd_identify(int drive)
{
        struct hd_cmd cmd;
        cmd.device = MAKE_DEVICE_REG(0, drive, 0);
        cmd.command = ATA_IDENTIFY;
        hd_cmd_out(&cmd);
        interrupt_wait();
        _port_read(REG_DATA, hdbuf, SECTOR_SIZE);
        //print_identify_info((u16*)hdbuf);
}


void print_identify_info(u16 *hdinfo)
{
        int i, k;
        char s[64];
        struct iden_info_ascii{
                int idx;
                int len;
                char *desc;
        } iinfo[]={{10,20,"HD SN"},{27,40,"HD Model"}};
        for(k=0;k<sizeof(iinfo)/sizeof(iinfo[0]);k++){
                char *p = (char*)&hdinfo[iinfo[k].idx];
                for(i=0;i<iinfo[k].len/2;i++){
                        s[i*2+1]=*p++;
                        s[i*2] = *p++;
                }
                s[i*2]=0;
                printk("%s: %s\n", iinfo[k].desc, s);
        }
        int capabilities = hdinfo[49];
        printk("LBA supported: %s\n", (capabilities & 0x0200)?"Yes":"No");
        int cmd_set_supported = hdinfo[83];
        printk("LBA48 supported: %s\n",(cmd_set_supported & 0x0400)?"Yes":"No");
        int sectors = ((int)hdinfo[61]<<16) + hdinfo[60];
        printk("HD size: %dMB\n", sectors * 512 / 1000000);
}


/**
 * output a command struct ptr.
 */
void hd_cmd_out(struct hd_cmd * cmd)
{
        if(!waitfor(REG_STATUS, STATUS_BSY, 0, HD_TIMEOUT)){
                panic("hd error.");
        }
        /* Activate the Interrupt Enable (nIEN) bit */
        _out_byte(REG_DEV_CTRL, 0);
        /* Load required parameters in the Command Block Registers */
        _out_byte(REG_FEATURES, cmd->features);
        _out_byte(REG_NSECTOR,  cmd->count);
        _out_byte(REG_LBA_LOW,  cmd->lba_low);
        _out_byte(REG_LBA_MID,  cmd->lba_mid);
        _out_byte(REG_LBA_HIGH, cmd->lba_high);
        _out_byte(REG_DEVICE,   cmd->device);
        /* Write the command code to the Command Register */
        _out_byte(REG_CMD,     cmd->command);
}
