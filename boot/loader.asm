;这里可以突破512kb的限制了
;loader.asm中需要做至少两件事：
;	1.加载内核入neicun
;	2.跳入保护模式
; 0x0100 在boot.asm中会将loader.bin放到0x9000:0x0100处，所以这里org 0x0100
org 0x0100
jmp LOADER_START
%include	"fat12hdr.inc"
%include	"memmap.inc"
%include	"pm.inc"
BaseOfStack	equ	BaseOfLoaderStack;0x0100	;栈地址 0x800000 - 0x80100作为loader的栈使用
BaseOfLoaded	equ	BaseOfKernelFile;0x8000		;内核被加载到的段地址
OffsetOfLoaded	equ	OffsetOfKernelFile;0x0000		;内核被加载到的偏移地址	
;GDT
;
LABEL_GDT:		Descriptor	0,	0,		0				;空描述符
LABEL_DESC_FLAT_C:	Descriptor	0,	0xFFFFF,	DA_CR|DA_32|DA_LIMIT_4K		;内存0-4G 数据段
LABEL_DESC_FLAT_RW:	Descriptor	0,	0xFFFFF,	DA_DRW|DA_32|DA_LIMIT_4K	;内存0-4G 可读写数据段
LABEL_DESC_VIDEO:	Descriptor	0xB8000,0xFFFF ,	DA_DRW|DA_DPL3			;显存首地址

GdtLen		equ	$-LABEL_GDT
;定义GDT指针
GdtPtr		dw	GdtLen-1				;指针前2字节 GDT长度
		dd	BaseOfLoaderPhyAddr + LABEL_GDT		;指针后4字节 GDT物理首地址
;GDT指针定义完成
;GDT 选择子
SelectorFlatC	equ	LABEL_DESC_FLAT_C	-	LABEL_GDT
SelectorFlatRW	equ	LABEL_DESC_FLAT_RW	-	LABEL_GDT
SelectorVideo	equ	LABEL_DESC_VIDEO	-	LABEL_GDT	+	SA_RPL3
;loader.bin开始
LOADER_START:
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, BaseOfStack

	;
	call clear_screen16

	;
	mov dl, 0	;the 0 idx str
	mov dh, 0	;disp_str row position
	call disp_str16

	xor ah, ah
	xor dl, dl
	int 0x13
	
	mov word[wSectorNo], SectorNoOfRootDirectory
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
	cmp word[wRootDirSizeForLoop], 0
	jz LABEL_NO_KERNELBIN	;循环完毕没有找到
	dec word[wRootDirSizeForLoop]

	mov ax, BaseOfLoaded
	mov es, ax			; es <-BaseOfLoaded 0x8000
	mov bx, OffsetOfLoaded		; bx <-OffsetOfLoaded
	mov ax, [wSectorNo]		; ax <-Root Directory中某sector号
	mov cl, 1			; cl <-1
	call read_sector16
	;
	mov si, KernelFileName		;ds:si -> "KERNEL  BIN"
	mov di, OffsetOfLoaded		;es:di -> BaseOfLoaded:0x0000
	cld	;
	;
	mov dx, 0x10			;Root Entry 占用32字节 一个扇区占用512字节，所以需要循环 512/32=16=0x10次
LABEL_SEARCH_FOR_KERNELBIN:
	cmp dx, 0
	jz LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR
	dec dx
	;
	mov cx, 11			;11是FAT12文件系统中文件名长度	
LABEL_CMP_FILENAME:
	cmp cx, 0
	jz LABEL_FILENAME_FOUND		;11个都比较了，说明找到
	dec cx
	lodsb				;ds:si -> al
	cmp al, byte[es:di]
	jz LABEL_GO_ON
	jmp LABEL_DIFFERENT		;有一个字符不相同，就找下一项
LABEL_GO_ON:
	inc di
	jmp LABEL_CMP_FILENAME		;继续寻找	

LABEL_DIFFERENT:
	and di, 0xffe0			;di & =e0 指向本条目开头
	add di,	0x20			;di+=0x20	下一个条目
	mov si,	KernelFileName
	jmp LABEL_SEARCH_FOR_KERNELBIN

LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR:
	add word[wSectorNo], 1
	jmp LABEL_SEARCH_IN_ROOT_DIR_BEGIN

LABEL_NO_KERNELBIN:
	mov dl, 2
	mov dh, 1
	call disp_str16	
	jmp $

LABEL_FILENAME_FOUND:
	;找到了
	mov ax, RootDirSectors
	and di, 0xFFF0
	push eax
	mov eax, [es:di + 0x001C]	;FAT12 Root Entry中偏移1c的是文件大小
	mov dword[dwKernelSize], eax	;保存文件大小
	pop eax
	
	add di, 0x001A
	mov cx, word[es:di]
	push cx
	add cx, ax
	add cx, DeltaSectorNo
	mov ax, BaseOfLoaded
	mov es, ax
	mov bx, OffsetOfLoaded
	mov ax, cx

LABEL_GOON_LOADING_FILE:
	push ax
	push bx
	mov ah, 0x0E
	mov al, '.'
	mov bl, 0x0F
	int 0x10
	pop bx
	pop ax

	mov cl, 1
	call read_sector16
	pop ax
	call get_FAT_entry16
	cmp ax, 0x0FFF
	jz LABEL_FILE_LOADED
	push ax
	mov dx, RootDirSectors
	add ax, dx
	add ax, DeltaSectorNo
	add bx, [BPB_BytsPerSec]
	jmp LABEL_GOON_LOADING_FILE

LABEL_FILE_LOADED:
	call kill_motor16
	
	mov dl,1
	mov dh,1
	call disp_str16
	;TODO 1 获取及其物理内存大小和分布
	;TODO 2 将ELF格式的kernel.bin对齐并放到BaseOfKernel:OffsetOfKernel处
	cli   ;先关中断，然后废除BIOS中断
	;TODO 3	进入32位保护模式 
	;下面准备跳入保护模式
	
	;加载GDTR
	lgdt [GdtPtr]
	;关中断
	;cli
	;打开地址线A20 实现32位寻址
	in al, 0x92
	or al, 00000010b
	out 0x92, al
	
	;准备切换到保护模式
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	
	;真正进入保护模式
	jmp dword SelectorFlatC:(BaseOfLoaderPhyAddr+LABEL_PM_START)
	;TODO 4跳入内核
	;jmp dword SelectorFlatC:KernelPhyAddr
	;jmp $

%include	"lib16.inc"	;实模式下的函数

;

KernelFileName	db	'KERNEL  BIN',0		;kernel file name:must length 8
;
dwKernelSize	dd	0			;保存文件大小
;
MessageLength	equ	9
;
Message:	db	'Loading  '
Message1:	db	':) Kernel'
Message2:	db	'No Kernel'


;---------------------------------------------------------------------
;TODO 32位模式下的处理都放到kernel中去做，未来删除掉这些
;保护模式下的代码
[SECTION .s32]
ALIGN 32
[BITS 32]
%include	'lib32.inc'
LABEL_PM_START:
	;设置显存 到gs
	mov ax, SelectorVideo
	mov gs, ax
	;数据段
	mov ax, SelectorFlatRW
	mov ds, ax
	mov es, ax
	;
	mov fs, ax	;FS寄存器指向当前活动线程的TEB结构（线程结构）
	mov ss, ax
	mov esp, TopOfStack

	;32保护模式下的显示字符串
	push MessagePM
	mov eax, [dwDispPos]
	push eax
	call disp_str
	pop eax
	mov [dwDispPos], ax 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	mov esi, MessagePM
;	mov edi, [dwDispPos]
;	mov ah, 0x07
;.1:
;	lodsb
;	test al, al
;	jz .2
;	cmp al, 0x0A	;是回车吗
;	jnz .3
;	push eax
;	mov eax, edi
;	mov bl, 160
;	div bl
;	and eax, 0xFF
;	inc eax
;	mov bl, 160
;	mul bl
;	mov edi, eax
;	pop eax
;	jmp .1
;.3:
;	mov [gs:edi], ax
	add edi, 2
;	jmp .1
;.2:
;	mov [dwDispPos], edi
	
	jmp $	


;保护模式下的数据
[SECTION .data]		;数据段
ALIGN	32
[BITS	32]
_MessagePM:	db	'In Protected Mode :)', 0x0A,'Hello Protected Mode', 0x00		;相对于此文件的偏移
MessagePM:	equ	BaseOfLoaderPhyAddr + _MessagePM;保护模式下，由于数据段基地址被描述符设定为0，所以偏移地址应该是就是在内存中的物理地址;物理地址如何计算才进入loader.bin时，cs（代码基地址寄存器）被boot.asm 最后的jmp修改为BaseOfLoader 0x9000， 随后将ds也设置为cs（数据基地址寄存器）的值，所以loader.bin中的数据也是相对于ds的偏移，所以计算物理地址为：ds:xx 也就是ds*0x10+xx
_dwDispPos:	dd	(80*2+0)*2	;屏幕第6行第0列
dwDispPos	equ	BaseOfLoaderPhyAddr + _dwDispPos	
;保存内存信息
;SECTION .data结束

[SECTION .gs]		;全局堆栈段
ALIGN	32
[BITS	32]
;32保护模式下堆栈就在数据段的末尾
StackSpace:	times 1024	db	0	;堆栈空间	1M
TopOfStack	equ	BaseOfLoaderPhyAddr + $	;栈顶
;SECTION .gs结束

