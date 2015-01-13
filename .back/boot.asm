org 0x7c00
mov ax,cs
mov ds,ax
mov es,ax
call clean_screen

;call change_mode

call disp_str
;hlt ;注释掉，因为如果hlt了，就不能响应ctrl+c退出
;jmp $-1
jmp $

change_mode:
	mov ah,0
	;mov al, 0x00 ;16色字符模式40 X 25
	mov al, 0x03 ;16色字符模式 80 X 25
	;mov al, 0x12 ;VGA图形模式,640X480X4位彩色模式
	;mov al, 0x13 ;VGA图形模式，320X200X8位彩色模式
	;mov al, 0x6a ;扩展VGA图形模式，800X600X4位彩色模式
	int 0x10
	ret

clean_screen:
	mov ax,0x0600
	mov bx,0x0000
	mov cx,0x0000
	mov dl, 79
	mov dh, 24
	int 0x10
	ret
	
disp_str:
	mov ax, BootMessage
	mov bp, ax
	mov cx,len
	mov ax, 0x1301 ;ah:0x13 显示服务功能号：显示字符串  al:0x01 显示模式
	;显示模式(AL)
	;    0x00:字符串只包含字符码，显示之后不更新光标位置，属性值在BL中
	;    0x01:字符串只包含字符码，显示之后更新光标位置，属性值在BL中
	;    0x02:字符串包含字符码及属性值，显示之后不更新光标位置
	;    0x03:字符串包含字符码及属性值，显示之后更新光标位置
	
	mov bh, 0x00 ;视频页
	;黑底
	;mov bl, 0x00 ;黑
	;mov bl, 0x01 ;蓝
	;mov bl, 0x02 ;绿
	;mov bl, 0x03 ;淡蓝
	;mov bl, 0x04 ;红
	;mov bl, 0x05 ;紫
	;mov bl, 0x06 ;黄
	mov bl, 0x07 ;白
	;mov bl, 0x08 ;灰
	;mov bl, 0x09 ;蓝
	;mov bl, 0x0a ;亮绿
	;mov bl, 0x0b ;亮蓝
	;mov bl, 0x0c ;亮红
	;mov bl, 0x0d ;亮紫
	;mov bl, 0x0e ;亮黄
	;mov bl, 0x0f ;亮白
	;蓝底
	;mov bl, 0x10 ;黑字
	;mov bl, 0x11 ;蓝字
	;; ... ...
	;绿底
	;mov bl, 0x20 ;黑字
	;
	mov dh, 0x00 ;起始行
	mov dl, 0x00 ;起始列
	int 0x10 ;BIOS 显示服务中断
	ret
BootMessage:
	db "hello, OS world!\nI'm your father!!!!"
len equ $-BootMessage
times 510 - ($-$$) db 0
dw 0xaa55
