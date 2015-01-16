boot.asm
;nasm -o boot.bin boot.asm
;软盘引导扇区
org 0x7c00

;加载boot.bin使用的常量
BaseOfStack		equ	0x7c00	;栈基址（从0x7c00开始向底地址生长，也就是栈顶地址<0x7c00）
BaseOfLoaded		equ	0x9000	;loader.bin被加载到的位置 --- 段地址
OffsetOfLoaded		equ	0x0100	; loader.bin被加载到的位置 --- 偏移地址 0x9000:0x0x100这里空闲

;开始
jmp short BOOT_START	;Start to boot
nop			;fat12开始的jmp段长度要求为3

%include	"fat12hdr.inc"	;磁盘的一些信息定义在fat12hdr.inc中

BOOT_START:
	;实模式下
	mov ax,cs
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov sp,BaseOfStack
	;清屏
	call clear_screen
	;显示boot
	mov dh, 0
	mov dl ,0
	call disp_str
	xor ah, ah
	xor dl, dl
	int 0x13	;软驱复位
	;int wSectorNo = SectorNoOfRootDirectory	
	mov word[wSectorNo], SectorNoOfRootDirectory
LABEL_SEARCH_IN_ROOT_DIR_BEGIN: ;开始在根目录中搜索loader.bin
	;while(wRootDirSizeForLoop > 0)
	cmp word[wRootDirSizeForLoop], 0 ;循环的时候wRootDirSizeForLoop从19不断减少，减少到0表示没有找到 
	jz LABEL_NO_LOADERBIN ;循环完没有找到
	;单次循环中
	dec word[wRootDirSizeForLoop] ;先减少
	mov ax, BaseOfLoaded
	;以下4个作为参数供read_sector使用
	mov es, ax		;es <- BaseOfLoaded
	mov bx, OffsetOfLoaded	;bx <- OffsetOfLoaded
	mov ax, [wSectorNo]	;ax <- Root Directory中某sector号
	mov cl, 1		;
	call read_sector
	
	mov si, LoaderFileName ;ds:si -> "LOADER    BIN"
	mov di, OffsetOfLoaded ;es:di -> BaseOfLoaded:0x0100
	cld ;使DF=0（DF Direction Flag 方向位）
	;
	mov dx, 0x10 ;循环的次数16次(Root Entry每一项占用32字节，一个扇区占用512字节，所以需要循环512/32=16=0x10次)
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
	mov dl, 2
	mov dh, 1	;显示idx为2的字符串
	call disp_str	;显示字符串
	jmp $
LABEL_FILENAME_FOUND:	;物理地址  (0x7cb6 可以break到这里)
	;如果找到了，此时0x9000:0x0100里保存的是有loader.bin的Root Directory的一个扇区，此时es:di(0x9000:di)保存的是找到的Root Entry
	mov ax, RootDirSectors
	and di, 0xFFE0	;di->当前条目开始 (此时di=0x012b & 0xffe0后= 0x0120正是一个root entry的开头
	add di, 0x001A	; di->首sector (此时di=0x0120 +0x001A 后=0x013A 0x001A是一个root entry中DIR_FstClus 此条木对应开始簇号，又因为在这个软盘中一簇对应一个扇区，所以即是文件第一个扇区数（这个扇区数是相对数据区来说的，真正的物理扇区号还需要进一步计算，同时这个也是FAT表的下表号）)
	mov cx, word[es:di]	;cx 此时保存的是文件loader.bin数据开始的第一个扇区号
	push cx	;保存此Sector在FAT中的序号
	;以下开始计算真正的物理扇区号
	add cx, ax
	add cx, DeltaSectorNo	;cl<- loader.bin 的起始扇区号（从0开始)
	mov ax, BaseOfLoaded
	mov es, ax 		;es <- BaseOfLoaded
	mov bx, OffsetOfLoaded	;bx <- OffsetOfLoaded 此时加载到es:di的Root Directory Sector已经没有作用，这块内存可以回收作为加载loader.bin数据使用
	mov ax, cx		;ax <- sector 号
LABEL_GOON_LOADING_FILE:
	;mov dh, 0
	;mov dl ,0x00
	;call disp_str 这里不能随便调用disp_str，因为里面改变亮ax的值，下面push ax 再pop出来时，是错误的值了
	;以下用于显示加载进度
	push ax
	push bx
	
	mov ah, 0x0E
	mov al, '.'
	mov bl, 0x0F
	int 0x10

	pop bx
	pop ax
	
	mov cl, 1
	call read_sector
	pop ax	;取出此Sector在FAT中的序号
	call get_FAT_entry
	cmp ax, 0x0FFF
	jz LABEL_FILE_LOADED ;表示加载完成
	push ax
	mov dx, RootDirSectors
	add ax, dx
	add ax, DeltaSectorNo
	add bx, [BPB_BytsPerSec]
	jmp LABEL_GOON_LOADING_FILE
LABEL_FILE_LOADED:
	mov dh,1
	mov dl,1
	call disp_str
	;call clear_screen
	jmp BaseOfLoaded:OffsetOfLoaded ;跳转到loader.bin的开始

%include	"func.inc"

;字符串
LoaderFileName	db 'LOADER  BIN',0

MessageLength	equ	9 ;为了减缓代码，所以字符串长度均为9
Message:	db	'Booting  '	;
Message1:	db	'Ready.   '
Message2:	db	'No LOADER'
;Message3:	db	'Read Fail'
times 510 - ($-$$) db 0
dw 0xaa55
