;这里可以突破512kb的限制了
;loader.asm中需要做至少两件事：
;	1.加载内核入neicun
;	2.跳入保护模式
; 0x0100 在boot.asm中会将loader.bin放到0x9000:0x0100处，所以这里org 0x0100
org 0x0100
BaseOfStack	equ	0x0100	;栈地址 0x9000 - 0x90100作为loader的栈使用
BaseOfKernel	equ	0x8000	;内核被加载到的段地址
OffsetOfKernel	equ	0x0000	;内核被加载到的偏移地址	
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
	jmp $


;functions
;------------------------------------------------------------;

;function:disp_str
;parameter:
;	dl <-- str idx
;	dh <-- row position
disp_str:
	mov ax, MessageLength
	mul dl	;mul the length of message
	add ax, LoaderMessage
	mov bp, ax
	mov ax, ds
	mov es, ax
	mov cx, MessageLength
	mov ax, 0x1301
	mov bx, 0x0007
	mov dl, 0x00
	int 0x10
	ret

;function:clear_screen
;
clear_screen:
	mov ax, 0x0200
	mov bx, 0x0000
	mov dx, 0x0000
	int 0x10
	
	mov ax, 0x0600
	mov bx, 0x0700
	mov cx, 0x0000
	mov dh, 24
	mov dl ,9
	int 0x10
	ret

;

KernelFileName	db	'KERNEL  BIN',0		;kernel file name:must length 8
;
MessageLength	equ	9
;
LoaderMessage:	db	'Loading  '
Message1:	db	'No Kernel'
	
