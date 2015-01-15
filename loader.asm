;这里可以突破512kb的限制了
; 0x0100 在boot.asm中会将loader.bin放到0x9000:0x0100处，所以这里org 0x0100
org 0x0100
	xor ax, ax
	mov es, ax
	mov ah, 0x0E
	mov al, 'L'
	mov bl, 0x0F
	int 0x10

	mov al, 'O'
	int 0x10
	
	mov al, 'A'
	int 0x10
	
	mov al, 'D'
	int 0x10

	mov al, 'E'
	int 0x10
	
	mov al, 'D'
	int 0x10

	mov ax, 0xb800	;字符模式显存首地址
	mov gs, ax	
	mov ah, 0x0F
	mov al, 'L'
	mov [gs:((80*0+39)*2)], ax	;屏幕第0行 第39列
	jmp $


;times 512*200 db 0
