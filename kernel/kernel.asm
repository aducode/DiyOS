;org 0x0000
;%include	'memmap.inc'
mov ax, 0x0200
mov bx, 0x0000
mov dx, 0x0000
int 0x10

mov ax, 0x0600
mov bx, 0x0700
mov cx, 0x0000
mov dh, 24
mov dl, 79
int 0x10

;mov ax, Message
mov bp ,ax
;mov ax ,BaseOfKernelFile
mov es, ax
;mov cx, MessageLength
mov ax, 0x1301
mov bx, 0x0007
mov dx, 0x0000
int 0x10
jmp $
Message:	db	':) You In Kernel Now!'
MessageLength	equ	$ - Message
