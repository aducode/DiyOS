;定义boot.asm和loader.asm都会用到的一些函数
; -----------------------------------------------------------------------------------
;函数 clear_screen
;作用：清空屏幕
;使用BIOS 0x10 中断清屏
clear_screen:
	;设置光标位置
	mov ax, 0x0200
	mov bx, 0x0000
	mov dx, 0x0000
	int 0x10

	mov ax,0x0600
	mov bx, 0x0700	;清屏的同时设置文字颜色
	mov cx, 0x0000
	mov dh, 24
	mov dl, 79
	int 0x10
	ret
;使用BIOS 0x10中断显示字符串
;----------------------------------------------------------------------------
; 函数名: DispStr
;----------------------------------------------------------------------------
; 作用:
;	显示一个字符串, 函数开始时 dl 中应该是字符串序号(0-based)
;	dh -> row			
disp_str:
	mov ax, MessageLength
	mul dl	;乘以9 字符窗长度，用于偏移
	add ax, Message
	mov bp, ax
	mov ax, ds
	mov es, ax
	mov cx, MessageLength
	mov ax, 0x1301
	mov bx, 0x0007
	mov dl, 0x00
	int 0x10
	ret	
;函数名：kill_motor
;作用：
;	关闭驱动马达
;kill_motor:
;	push dx
;	mov dx, 0x03F2
;	mov al, 0
;	out dx, al
;	pop dx
;	ret
; 函数名：read_sector
;-------------------------------------------------------------------------
;作用
;	从ax个Sector开始，将cl个Sector读入es:bx中
;	注意 bx 为16位寄存器 最大值0xFFFF
;	loader.asm中如果结尾有times 512*200 db 0 那么在fat表中不断的查，会连续使用130多个sector，dx不断累加，(测试中的结果是0xFF00 ，然后在int 0x13时，磁盘服务会出错，导致不断重试)
read_sector:
	;初始化变量
	;mov byte[wRetryCount],	ReadRetryCount
	; -----------------------------------------------------------------------
	; 怎样由扇区号求扇区在磁盘中的位置 (扇区号 -> 柱面号, 起始扇区, 磁头号)
	; -----------------------------------------------------------------------
	; 设扇区号为 x
	;                           ┌ 柱面号 = y >> 1
	;       x           ┌ 商 y ┤
	; -------------- => ┤      └ 磁头号 = y & 1
	;  每磁道扇区数     │
	;                   └ 余 z => 起始扇区号 = z + 1
	push bp
	mov bp, sp
	sub sp, 2
	
	mov byte[bp-2], cl
	push bx ;保存bx
	mov bl, [BPB_SecPerTrk] ;bl除数
	div bl 	;y 在 al中, z在ah中
	inc ah	;z++
	mov cl, ah	;cl <-起始扇区号
	mov dh, al	;dh <- y
	shr al, 1
	mov ch, al	;ch <- 柱面号
	and dh, 1	;dh & 1 磁头号
	pop bx		;回复bx
	;至此柱面号 起始扇区 磁头号 全部得到
	mov dl, [BS_DrvNum]	;驱动器号（0表示A盘）
.GoOnReading:
	mov ah, 2;
	mov al,byte[bp-2];byte[bp-2]中开始保存的是传过来的cl（cl个扇区) 寄存器不够，所以需要使用栈保存局部变量
	int 0x13
	jc .GoOnReading
;	jnc .ReadSuccess	;如果成功
;.ReadError:			;如果失败重试 
;	cmp byte[wRetryCount], 0
;	jz .ReadFail	;重试次数到达，失败
;	dec byte[wRetryCount]
;	jmp .GoOnReading ;如果读取出错CF会被置为1
			;这时就不停的读，直到成功位置
;.ReadSuccess:
	add sp,2
	pop bp
	ret	
.ReadFail:
	call clear_screen
	mov dh, 3
	mov dl, 0
	call disp_str
	jmp $	

;----------------------------------------------------------------------------
; 函数名: get_FAT_entry
;----------------------------------------------------------------------------
; 作用:
;	找到序号为 ax 的 Sector 在 FAT 中的条目, 结果放在 ax 中
;	需要注意的是, 中间需要读 FAT 的扇区到 es:bx 处, 所以函数一开始保存了 es 和 bx
get_FAT_entry:
	push es
	push bx
	push ax
	mov ax, BaseOfLoaded
	sub ax, 0x0100
	mov es, ax
	pop ax
	mov byte [bOdd], 0
	mov bx, 3
	mul bx
	mov bx, 2
	div bx
	cmp dx, 0
	jz LABEL_EVEN
	mov byte[bOdd], 1
LABEL_EVEN:
	;现在ax中是FAT Entry在FAT中的偏移量，下面来计算FAT Entry在哪个扇区中（FAT占用不止一个扇区，9个）
	xor dx, dx
	mov bx, [BPB_BytsPerSec]
	div bx; dx:ax / BPB_BytePerSec
	      ;ax<-商
	      ;dx<-余数
	push dx
	mov bx, 0;
	add ax, SectorNoOfFAT1
	mov cl, 2
	call read_sector
	pop dx
	add bx, dx
	mov ax, [es:bx]
	cmp byte[bOdd], 1
	jnz LABEL_EVEN_2
	shr ax, 4
LABEL_EVEN_2:
	and ax, 0x0FFF
LABEL_GET_FAT_ENTRY_OK:
	pop bx
	pop es
	ret


;都会用到的一些变量
wRootDirSizeForLoop	dw RootDirSectors	; Root Directory 占用扇区数
wSectorNo		dw 0			; 要读取的扇区号
bOdd			dw 0			; 奇数还是偶数 flag变量

;wRetryCount		db 0			; 磁盘读取重试次数
