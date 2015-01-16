%include	"bootlib.inc"

;函数名:kill_motor
;作用：
;	关闭软驱马达
kill_motor:
	push dx
	mov dx,	0x03F2
	mov al, 0
	out dx, al
	pop dx
	ret
