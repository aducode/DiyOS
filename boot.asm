org 0x7c00
mov ax,cs
mov ds,ax
mov es,ax
;call CleanScreen

;mov ah, 0
;mov al, 0x6a
;int 0x10

call DispStr
;hlt ;注释掉，因为如果hlt了，就不能响应ctrl+c退出
;jmp $-1
jmp $
CleanScreen:
	mov ax,0x0600
	mov bx,0x0000
	mov cx,0x0000
	mov dl, 79
	mov dh, 24
	int 0x10
	ret
	
DispStr:
	mov ax, BootMessage
	mov bp, ax
	mov cx,len
	mov ax, 0x1301
	
	mov bx, 0x0001
;	mov dh, 0x02
	mov dl, 0x00
	int 0x10
	ret
BootMessage:
	db "hello, OS world!\nI'm your father!!!!"
len equ $-BootMessage
times 510 - ($-$$) db 0
dw 0xaa55
