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
;这里只是临时设置的GDT，未来整理内存的时候还需要重新设置
BaseOfStack	equ	BaseOfLoaderStack;0x0100	;栈地址 0x800000 - 0x80100作为loader的栈使用
BaseOfLoaded	equ	BaseOfKernelFile;0x8000		;内核被加载到的段地址
OffsetOfLoaded	equ	OffsetOfKernelFile;0x0000		;内核被加载到的偏移地址	
;GDT
;
LABEL_GDT:		Descriptor	0,	0,		0				;空描述符
LABEL_DESC_FLAT_C:	Descriptor	0,	0xFFFFF,	DA_CR|DA_32|DA_LIMIT_4K		;内存0-4MB 数据段
LABEL_DESC_FLAT_RW:	Descriptor	0,	0xFFFFF,	DA_DRW|DA_32|DA_LIMIT_4K	;内存0-4MB 可读写数据段
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

;获取内存数
	mov ebx, 0
	mov di, _MemChkBuf	;es:di 指向下一个地址范围描述结构
.MemChkLoop:
	mov eax, 0xE820		;eax = 0x0000E820
	mov ecx, 20		;ecx = 地址范围描述结构大小
	mov edx, 0x0534D4150	;edx = 'SMAP'
	int 0x15		;
	jc .MemChkFail
	add di, 20
	inc dword [_dwMCRNumber];dwMCRNumber = ARDS 个数
	cmp ebx, 0
	jne .MemChkLoop
	jmp .MemChkOK
.MemChkFail:
	mov dword [_dwMCRNumber], 0
.MemChkOK:
	;下面开始在软盘中寻找kernel
	;复位软驱
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
	;物理内存大小保存在
	;TODO 2 将ELF格式的kernel.bin对齐并放到BaseOfKernel:OffsetOfKernel处
	;call init_kernel	;此处不能在实模式下进行elf内核对齐的工作，因为实模式下寻址空间有限，需要不断的操作ds才能移动数据
	;所以还是在32位保护模式下进行移动内核的操作（32位保护模式下，ds被设置成0-64MB的物理内存空间，所以移动起来比较方便）
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
	;jmp dword SelectorFlatC:KernelEntryPointPhyAddr
	;jmp $

%include	"lib16.inc"	;实模式下的函数
;对齐elf到KernelPhyAddr
;init_kernel:
;	;push ds
;	mov ax, BaseOfKernelFile
;	mov ds, ax
;	xor esi, esi
;	mov cx, word[0x2c]	; cx <- pELEHdr->e_phnum
;	movzx ecx, cx	;ecx高位填充0 低位等于cx
;	mov esi,[0x1c]		; si <- pELFHdr->e_phoff
	;add esi, BaseOfKernelFilePhyAddr			; 将ELF中的偏移地址映射到物理地址
;.1:
;	mov eax, [si+0]
;	cmp eax, 0
;	jz .2
;	push dword[esi+0x10]	;program size
;	mov eax, [esi+0x04]
;	;add eax, BaseOfKernelFilePhyAddr
;	push eax		;program addr
;	push dword[esi+0x08]
;	call memcpy
;	add esp, 12
;.2:
;	add esi, 0x20	;esi +=pELFHdr->e_phentsize
;	dec ecx
;	jnz .1
;	;pop ds
;	mov ax, BaseOfLoader	;ds切换回loader.bin
;	mov ds, ax
;	ret

;内存拷贝
;参数
;	void * memcpy(void * es:pDest, void * ds:pSrc, int iSize);
;memcpy:
;	mov ax, BaseOfLoader
;	mov ds, ax
;	push ebp
;	mov ebp, esp
;	push esi
;	push edi
;	push ecx
;
;	mov edi, [ebp+8]		;destination
;	mov esi,[ebp+12]	;source
;	mov ecx, [ebp+16]	;counter	
;.1:
;	cmp ecx, 0
;	jz .2
;	
;	mov al, [ds:esi]
;	inc esi
;	
;	mov byte[es:edi], al	
;	inc edi
;	
;	dec ecx
;	jmp .1
;.2:
;	mov eax, [ebp+8]	;返回值
;	pop ecx
;	pop edi
;	pop esi
;	mov esp, ebp
;	pop ebp
;
;	mov ax, BaseOfKernelFile
;	mov ds, ax	
;	ret
	

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
	;检查内存，并启动分页
	call _check_mem
	call _setup_paging
	;32保护模式下的显示字符串
	push MessagePM
	mov eax, [dwDispPos]
	push eax
	call disp_str
	pop eax
	mov [dwDispPos], ax ;改变光标位置
	;对齐之前先清屏
	;call clear_screen16	;这里不起作用了，因为在跳到32位保护模式之前已经关中断了
	;对齐elf到KernelEntryPointPhyAddr
	xor esi, esi
	mov cx, word[BaseOfKernelFilePhyAddr + 0x2C]
	movzx ecx, cx
	mov esi, [BaseOfKernelFilePhyAddr + 0x1C]
	add esi, BaseOfKernelFilePhyAddr
.BEGIN:
	mov eax, [esi+0]
	cmp eax, 0
	jz .END
	push dword[esi+0x10]	;size
	mov eax, [esi+0x04]
	add eax, BaseOfKernelFilePhyAddr
	push eax		;src
	push dword[esi+0x08]	;dst
	call memcpy
	add esp, 12
.END:
	add esi, 0x20
	dec ecx
	jnz .BEGIN
	
	;跳到内核	
	jmp SelectorFlatC:KernelEntryPointPhyAddr


_check_mem:
	push esi
	push edi
	push ecx
	mov esi, MemChkBuf
	mov ecx, [dwMCRNumber]
.loop:
	mov edx, 5
	mov edi, ARDStruct
.1:
	mov eax, [esi]	
	stosd
	add esi, 4
	dec edx
	cmp edx, 0
	jnz .1
	cmp dword[dwType], 1		;书上说type可能1 2(不可用) 但是我得到3类型了,3也作为不可用的
	jne .2
	mov eax, [dwBaseAddrLow]
	add eax, [dwLengthLow]
	cmp eax, [dwMemSize]
	jb .2   ;<
	mov [dwMemSize], eax
.2:
	loop .loop
;	cmp ecx, 0
;	je .3
;	dec ecx
;	jmp .loop	
;.3:
	pop ecx
	pop edi
	pop esi
	ret
;启动分页机制
_setup_paging:
	xor edx, edx
	mov eax, [dwMemSize]
	mov ebx, 0x400000	;一个页表对应内存大小
	div ebx
	mov ecx, eax	;此时ecx为页表个数，PDE应该个数
	test edx, edx
	jz .no_remainder
	inc ecx		;如果余数不为0就增加一个页表
.no_remainder:
	push ecx	;暂存页表个数
	; 为简化处理, 所有线性地址对应相等的物理地址. 并且不考虑内存空洞.
	; 首先初始化页目录
	mov	ax, SelectorFlatRW
	mov	es, ax
	mov	edi, PageDirBase	; 此段首地址为 PageDirBase
	xor	eax, eax
	mov	eax, PageTblBase | PG_P  | PG_USU | PG_RWW
.1:
	stosd
	add eax, 4096	;所有页表在内存中是连续的
	loop .1
	;再初始化所有表项
	pop eax	;页表个数
	mov ebx, 1024	;每个页表1024个PTE
	mul ebx
	mov ecx, eax
	mov edi, PageTblBase
	xor eax, eax
	mov eax, PG_P|PG_USU|PG_RWW
.2:
	stosd
	add eax, 4096
	loop .2
	mov eax, PageDirBase
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	jmp short .3
.3:
	nop
	ret
;保护模式下的数据
[SECTION .data]		;数据段
ALIGN	32
[BITS	32]
_MessagePM:	db	'In Protected Mode :)', 0x0A,'Now init KERNEL', 0x00		;相对于此文件的偏移
MessagePM:	equ	BaseOfLoaderPhyAddr + _MessagePM;保护模式下，由于数据段基地址被描述符设定为0，所以偏移地址应该是就是在内存中的物理地址;物理地址如何计算才进入loader.bin时，cs（代码基地址寄存器）被boot.asm 最后的jmp修改为BaseOfLoader 0x9000， 随后将ds也设置为cs（数据基地址寄存器）的值，所以loader.bin中的数据也是相对于ds的偏移，所以计算物理地址为：ds:xx 也就是ds*0x10+xx
_dwDispPos:	dd	(80*2+0)*2	;屏幕第2行第0列
dwDispPos	equ	BaseOfLoaderPhyAddr + _dwDispPos	
;保存内存信息
_MemChkBuf:	times	256 db 0
MemChkBuf	equ	BaseOfLoaderPhyAddr + _MemChkBuf
_dwMCRNumber	dd	0	;Memory Check Result
dwMCRNumber	equ	BaseOfLoaderPhyAddr + _dwMCRNumber
_dwMemSize:	dd	0	;记录内存信息
dwMemSize	equ	BaseOfLoaderPhyAddr + _dwMemSize
_ARDStruct:
	_dwBaseAddrLow:		dd	0
	_dwBaseAddrHigh:	dd	0
	_dwLengthLow:		dd	0
	_dwLengthHigh:		dd	0
	_dwType:		dd	0
ARDStruct		equ	BaseOfLoaderPhyAddr + _ARDStruct
	dwBaseAddrLow	equ	BaseOfLoaderPhyAddr + _dwBaseAddrLow
	dwBaseAddrHigh	equ	BaseOfLoaderPhyAddr + _dwBaseAddrHigh
	dwLengthLow	equ	BaseOfLoaderPhyAddr + _dwLengthLow
	dwLengthHigh	equ	BaseOfLoaderPhyAddr + _dwLengthHigh
	dwType		equ 	BaseOfLoaderPhyAddr + _dwType
;SECTION .data结束

[SECTION .gs]		;全局堆栈段
ALIGN	32
[BITS	32]
;32保护模式下堆栈就在数据段的末尾
StackSpace:	times 1024	db	0	;堆栈空间	1KB
TopOfStack	equ	BaseOfLoaderPhyAddr + $	;栈顶
;SECTION .gs结束

