BS_OEMName	DB	'DIYCOM  '	;长度必须8字节
BPB_BytsPerSec	DW	512		;每扇区字节数
BPB_SecPerClus	DB	1		;每簇多少扇区
BPB_RsvdSecCnt	DW	1		;Boot 记录占用多少扇区
BPB_NumFATs	DB	2		;共有多少FAT表
BPB_RootEntCnt	DW	224		;根目录文件数最大值
BPB_TotSec16	DW	2880		;逻辑扇区总数
BPB_Media	DB	0xF0		;媒体描述符
BPB_FATSz16	Dw	9		;每FAT扇区数
BPB_SecPerTrk	DW	18		;每磁道扇区数
BPB_NumHeads	DW	2		;磁头数
BPB_HiddSec	DD	0		;隐藏扇区数
BPB_TotSec32	DD	0		;wTotalSectorCount为0时这个记录扇区数
BS_DrvNum	DB	0		;中断13的驱动器号
BS_Reserved1	DB	0		;未使用
BS_BootSig	DB	0x29		;扩展引导标记
BS_VolID	DB	0		;卷序列号
BS_VolLab	DB	'DiyOS_V0.01'	;卷标，必须11字节
BS_FileSysType	DB	'FAT12   '		;文件系统类型，必须8个字节

;常量
BaseOfStack		equ	0x7c00	;栈基址（从0x7c00开始向底地址生长，也就是栈顶地址<0x7c00）
BaseOfLoader		equ	0x9000	;loader.bin被加载到的位置 --- 段地址
OffsetOfLoader		equ	0x0100	; loader.bin被加载到的位置 --- 偏移地址 0x9000:0x0x100这里空闲
RootDirSectors		equ	14	;根目录占用的扇区数
SectorNoOfRootDirectory equ 	19	;Root Directory的第一个扇区号

SectorNoOfFAT1		equ 	1	;FAT1 的第一个扇区号 = BPB_RsvdSecCnt
DeltaSectorNo		equ	17	;DeltaSectorNo = Boot占用扇区数1 + (FAT数2 * FAT占用扇区数9) - 2 =17

;ReadRetryCount		equ	5	;磁盘读取重试次数
