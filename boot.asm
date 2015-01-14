;boot.asm
;nasm -o boot.bin boot.asm
;软盘引导扇区
org 0x7c00

;常量
BaseOfStack		equ	0x7c00	;栈基址（从0x7c00开始向底地址生长，也就是栈顶地址<0x7c00）
BaseOfLoader		equ	0x9000	;loader.bin被加载到的位置 --- 段地址
OffsetOfLoader		equ	0x0100	; loader.bin被加载到的位置 --- 偏移地址 0x9000:0x0x100这里空闲
RootDirSectors		equ	14	;根目录占用的扇区数
SectorNoOfRootDirectory equ 	19	;Root Directory的第一个扇区号

SectorNoOfFAT1		equ 	1	;FAT1 的第一个扇区号 = BPB_RsvdSecCnt
;DeltaSectorNo		equ	17	;



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
	;清屏
	;call clear_screen
	;实模式下
	mov ax,cs
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov sp,BaseOfStack
	xor ah, ah
	xor dl, dl
	int 0x13	;软驱复位
	;int wSectorNo = SectorNoOfRootDirectory	
	mov word[wSectorNo], SectorNoOfRootDirectory
LABEL_SEARCH_IN_ROOT_DIR_BEGIN: ;开始在根目录中搜索loader.bin
	;while(wRootDirSizeForLoop > 0)
	;mov ax, word[wRootDirSizeForLoop]
	;mov ax, 2
	;call debug
	;mov dx, word[wRootDirSizeForLoop]
	;and dx, 0x00FF
	;call disp_str
	cmp word[wRootDirSizeForLoop], 0 ;循环的时候wRootDirSizeForLoop从19不断减少，减少到0表示没有找到 
	jz LABEL_NO_LOADERBIN ;循环完没有找到
	;单次循环中
	dec word[wRootDirSizeForLoop] ;先减少
	mov ax, BaseOfLoader
	;以下4个作为参数供read_sector使用
	mov es, ax		;es <- BaseOfLoader
	mov bx, OffsetOfLoader	;bx <- OffsetOfLoader
	mov ax, [wSectorNo]	;ax <- Root Directory中某sector号
	mov cl, 1		;
	call read_sector
	
	mov si, LoaderFileName ;ds:si -> "LOADER    BIN"
	mov di, OffsetOfLoader ;es:di -> BaseOfLoader:0x0100
	cld ;使DF=0（DF Direction Flag 方向位）
	;
	mov dx, 0x20 ;循环的次数16次
LABEL_SEARCH_FOR_LOADERBIN:	;在加载到es:bx中的一个扇区中的32个Root Entry中寻找loader.bin
	cmp dx, 0	;循环次数控制
	jz LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR ;在Root Directory的全部扇区中继续寻找loader.bin（这里是14个扇区）
	dec dx
	;
	mov cx, 11	;循环比较的次数，11是因为FAT12文件系统中文件名长度为11（8文件名+3后缀名）
LABEL_CMP_FILENAME:
	cmp cx, 0
	jz LABEL_FILENAME_FOUND	;如果比较了11个字符都相等，表示找到
	dec cx
	;汇编语言中，串操作指令LODSB/LODSW是块装入指令，其具体操作是把SI指向的存储单元读入累加器,LODSB就读入AL,LODSW就读入AX中,然后SI自动增加或减小1或2.其常常是对数组或字符串中的元素逐个进行处理。
	lodsb	;ds:si -> al ;开始写错成loadsb了，找了好久的错误。（错误的会被当作标签，编译器不会提示错误）
	cmp al, byte[es:di]
	jz LABEL_GO_ON
	jmp LABEL_DIFFERENT ;只要发现不一样的字符串就表示不是我们要找的

LABEL_GO_ON:
	inc di
	jmp LABEL_CMP_FILENAME	;继续循环

LABEL_DIFFERENT:
	and di, 0xffe0	; di &= e0 为了让它指向本条目开头
	add di, 0x20	; di+=0x20 下一个目录条目 在Root Directory中每一个每一个目录条目位32Byte 也就是0x20
	mov si, LoaderFileName
	jmp LABEL_SEARCH_FOR_LOADERBIN	;跳到单个扇区内的32个Root Entry中继续寻找

LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR:
	add word[wSectorNo], 1
	jmp LABEL_SEARCH_IN_ROOT_DIR_BEGIN
 
LABEL_NO_LOADERBIN:
	call clear_screen
	mov dl, 0
	mov dh, 2	;显示idx为2的字符串
	call disp_str	;显示字符串
	;显示
	;call clean_screen
	;call disp_str
	jmp $
LABEL_FILENAME_FOUND:
	;mov dl,0
	;mov dh,1
	;call disp_str
	jmp $

;使用BIOS 0x10 中断清屏
clear_screen:
	mov ax,0x0600
	mov bx, 0x0000
	mov cx, 0x0000
	mov dh, 24
	mov dl, 79
	int 0x10
	ret
;使用BIOS 0x10中断显示字符串
;----------------------------------------------------------------------------
; 函数名: DispStr
;----------------------------------------------------------------------------
; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
;	dl -> row			
disp_str:
	mov ax, MessageLength
	mul dh	;乘以9 字符窗长度，用于偏移
	add ax, BootMessage
	mov bp, ax
	mov ax, ds
	mov es, ax
	mov cx, MessageLength
	mov ax, 0x1301
	mov bx, 0x0007
	mov dh, dl
	mov dl, 0x00
	int 0x10
	ret	
;	mov ax, BootMessage
;	mov bp, ax
;	mov cx, len
;	mov ax, 0x1301
;	mov bh, 0x00
;	mov bl, 0x07
;	mov dx, 0x0000
;	int 0x10
;	ret
;使用BIOS 0x13中断操作磁盘

;BootMessage:
;	db "hello world!"
;len equ $-BootMessage
;变量

; 函数名：read_sector
;-------------------------------------------------------------------------
;作用
;	从ax个Sector开始，将cl个Sector读入es:bx中
read_sector:
	; -----------------------------------------------------------------------
	; 怎样由扇区号求扇区在磁盘中的位置 (扇区号 -> 柱面号, 起始扇区, 磁头号)
	; -----------------------------------------------------------------------
	; 设扇区号为 x
	;                           ┌ 柱面号 = y >> 1
	;       x           ┌ 商 y ┤
	; -------------- => ┤      └ 磁头号 = y & 1
	;  每磁道扇区数     │
	;                   └ 余 z => 起始扇区号 = z + 1
	push bp
	mov bp, sp
	sub sp, 2
	
	mov byte[bp-2], cl
	push bx ;保存bx
	mov bl, [BPB_SecPerTrk] ;bl除数
	div bl 	;y 在 al中, z在ah中
	inc ah	;z++
	mov cl, ah	;cl <-起始扇区号
	mov dh, al	;dh <- y
	shr al, 1
	mov ch, al	;ch <- 柱面号
	and dh, 1	;dh & 1 磁头号
	pop bx		;回复bx
	;至此柱面号 起始扇区 磁头号 全部得到
	mov dl, [BS_DrvNum]	;驱动器号（0表示A盘）
.GoOnReading:
	mov ah, 2;
	mov al,byte[bp-2];byte[bp-2]中开始保存的是传过来的cl（cl个扇区) 寄存器不够，所以需要使用栈保存局部变量
	int 0x13
	jc .GoOnReading ;如果读取出错CF会被置为1
			;这时就不停的读，直到成功位置
	add sp,2
	pop bp
	ret	


;----------------------------------------------------------------------------
; 函数名: get_FAT_entry
;----------------------------------------------------------------------------
; 作用:
;	找到序号为 ax 的 Sector 在 FAT 中的条目, 结果放在 ax 中
;	需要注意的是, 中间需要读 FAT 的扇区到 es:bx 处, 所以函数一开始保存了 es 和 bx
get_FAT_entry:
	push es
	push bx
	push ax
	mov ax, BaseOfLoader
	sub ax, 0x0100
	mov es, ax
	pop ax
	mov byte [bOdd], 0
	mov bx, 3
	mul bx
	mov bx, 2
	div bx
	cmp dx, 0
	jz LABEL_EVEN
	mov byte[bOdd], 1
LABEL_EVEN:
	;现在ax中是FAT Entry在FAT中的偏移量，下面来计算FAT Entry在哪个扇区中（FAT占用不止一个扇区，9个）
	xor dx, dx
	mov bx, [BPB_BytsPerSec]
	div bx; dx:ax / BPB_BytePerSec
	      ;ax<-商
	      ;dx<-余数
	push dx
	mov bx, 0;
	add ax, SectorNoOfFAT1
	mov cl, 2
	call read_sector
	pop dx
	add bx, dx
	mov ax, [es:bx]
	cmp byte[bOdd], 1
	jnz LABEL_EVEN_2
	shr ax, 4
LABEL_EVEN_2:
	and ax, 0x0FFF
LABEL_GET_FAT_ENTRY_OK:
	pop bx
	pop es
	ret
	
		

	
wRootDirSizeForLoop	dw RootDirSectors	; Root Directory 占用扇区数
wSectorNo		dw 0			; 要读取的扇区号
bOdd			dw 0			; 奇术还是偶数 flag变量
;字符串
LoaderFileName	db 'LOADER  BIN',0

MessageLength	equ	9 ;为了减缓代码，所以字符串长度均为9
BootMessage:	db	'Booting  '	;
Message1:	db	'Ready.   '
Message2:	db	'No LOADER'
times 510 - ($-$$) db 0
dw 0xaa55
