;显示的颜色
disp_str_color	equ 0x0F
;保护模式下32寄存器的函数
;显示字符串
;参数in:
;	1.字符串地址压栈
;	2.显存地址
;参数out：
;	pop 当前显存地址
disp_str:
	push ebp
	mov ebp, esp
	push ebx
	push esi
	push edi
	;栈向低地址方向增长
	;           <- esp 继续压栈ebx esi edi之后     address:LOW
	;|--edi---|                                          |
	;|--esi---|                                          |
	;|--ebx---|                                          |
	;            <- ebp ;mov ebp, esp之后                |
	;|---ebp--|                                          |
	;|-param2-|                                          |
	;|-param1-|                                          |
	;|--------|   <- 栈基址 		       address:HIGH
	mov esi, [ebp + 12]	;字符串地址param1  ebp+12 是因为ebp长度8 param2长度4 8+4=12	
	mov edi, [ebp + 8]	;显存地址param2    ebp+8 是因为ebp的长度为8Byte
	mov ah, disp_str_color
	
.1:
	lodsb
	test al, al
	jz .2
	cmp al, 0x0A	;是回车吗
	jnz .3
	push eax
	mov eax, edi
	mov bl, 160
	div bl
	and eax, 0xFF
	inc eax
	mov bl, 160
	mul bl
	mov edi, eax
	pop eax
	jmp .1
.3:
	mov [gs:edi], ax
	add edi, 2
	jmp .1
.2:
	mov [ebp+8],edi
	
	pop edi
	pop esi
	pop ebx
	pop ebp
	ret

;memcpy
;内存拷贝void * memcpy(void * es:pDest, void *ds:pSrc, int iSize);
memcpy:
	push ebp
	mov ebp, esp
	push esi
	push edi
	push ecx
	
	mov edi, [ebp+8]	;Destination
	mov esi, [ebp+12]	;Source
	mov ecx, [ebp+16]	;counter
.1:
	cmp ecx, 0
	jz .2
	
	mov al,[ds:esi]
	inc esi
	mov byte[es:edi], al	
	inc edi
	
	dec ecx
	jmp .1
.2:
	mov eax, [ebp+8]
	pop ecx
	pop edi
	pop esi
	mov esp, ebp
	pop ebp
	ret
