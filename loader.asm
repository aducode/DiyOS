;这里可以突破512kb的限制了
;loader.asm中需要做至少两件事：
;	1.加载内核入neicun
;	2.跳入保护模式
; 0x0100 在boot.asm中会将loader.bin放到0x9000:0x0100处，所以这里org 0x0100
org 0x0100
%include	"memmap.inc"
BaseOfStack	equ	BaseOfLoaderStack;0x0100	;栈地址 0x800000 - 0x80100作为loader的栈使用
BaseOfLoaded	equ	BaseOfKernel;0x8000		;内核被加载到的段地址
OffsetOfLoaded	equ	OffsetOfKernel;0x0000		;内核被加载到的偏移地址	
jmp LOADER_START
%include	"fat12hdr.inc"
;loader.bin开始
LOADER_START:
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, BaseOfStack

	;
	call clear_screen

	;
	mov dl, 0	;the 0 idx str
	mov dh, 0	;disp_str row position
	call disp_str

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
	call read_sector
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
	call disp_str	
	jmp $

LABEL_FILENAME_FOUND:
	;找到了
	mov dl,1
	mov dh,1
	call disp_str
	jmp $

%include	"lib.inc"

;

KernelFileName	db	'KERNEL  BIN',0		;kernel file name:must length 8
;
MessageLength	equ	9
;
Message:	db	'Loading  '
Message1:	db	':) Kernel'
Message2:	db	'No Kernel'
	
