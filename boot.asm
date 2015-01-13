;boot.asm
;nasm -o boot.bin boot.asm
;软盘引导扇区
org 0x7c00
jmp short BOOT_START	;Start to boot
nop			;fat12开始的jmp段长度要求为3
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

BOOT_START:
	;实模式下
	mov ax,cs
	mov ds,ax
	mov es,ax
	;显示
	call clean_screen
	call disp_str
	jmp $
clean_screen:
	mov ax,0x0600
	mov bx, 0x0000
	mov cx, 0x0000
	mov dh, 24
	mov dl, 79
	int 0x10
	ret

disp_str:	
	mov ax, BootMessage
	mov bp, ax
	mov cx, len
	mov ax, 0x1301
	mov bh, 0x00
	mov bl, 0x07
	mov dx, 0x0000
	int 0x10
	ret
BootMessage:
	db "hello world!"
len equ $-BootMessage
times 510 - ($-$$) db 0
dw 0xaa55
