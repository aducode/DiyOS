;汇编函数库
%include	"kernel.inc"
[section .text]
global _memcpy          ;void _memcpy(void * dest, void * src, int length);
global _memset		;void _memset(void * p_dst, char ch, int size);
global _hlt
global _disp_str
global _clean
global _out_byte
global _in_byte
;-----------------------------------------------------------------
;定义一些函数供c语言使用
;void out_byte(u16 port, u8 value)
_out_byte:
	mov edx, [esp + 4]	;port
	mov al, [esp + 4 + 4]	;value
	out dx, al
	nop
	nop	;一点延迟
	ret


;u8 in_byte(u16 port)
_in_byte:
	mov edx, [esp + 4]	;port
	xor eax, eax
	in al, dx
	nop
	nop
	ret	;返回值在eax中
_memcpy:
	push ebp
	mov ebp, esp
	
	push esi
	push edi	
	push ecx
	
	mov edi, [ebp+8]
	mov esi, [ebp+12]
	mov ecx, [ebp+16]

.1:
	cmp ecx, 0
	jz .2
	
	mov al ,[ds:esi]
	inc esi
	
	mov byte[es:edi], al
	inc edi
	
	dec ecx
	jmp .1
.2:
	mov eax, [ebp+8]	;返回值
	
	pop ecx
	pop edi
	pop esi
	mov esp, ebp
	pop ebp
	
	ret

;
_memset:
	push ebp
	mov ebp, esp
	
	push esi
	push edi
	push ecx
	
	mov edi, [ebp + 8]	;dst
	mov edx, [ebp +12]	;char to be putted
	mov ecx, [ebp +16]	;counter
.1:
	cmp ecx, 0
	jz .2
	
	mov byte[edi], dl
	inc edi

	dec ecx
	jmp .1
.2:
	pop ecx
	pop edi
	pop esi
	mov esp, ebp
	pop ebp
	
	ret		
		
;
_hlt:
	hlt
	ret


;函数：void disp_str(char * str, int row, int column, u8 color)
;loader.bin中已经将显存基址设置在gs寄存器中
_disp_str:
	push ebp
	mov ebp, esp
	push ebx
	push ecx
	push esi
	push edi
	;获取参数
	;c语言调用压栈顺序
	;栈内顺序
	;str
	;row
	;column
	mov esi, [ebp+8]	;字符串地址
	mov eax, [ebp+12]	;行
	mov edx, [ebp+16]	;列
	mov ecx, [ebp+20]	;color
	;计算显存地址
	mov bx,80
	mul bx
	add eax,edx
	mov bx, 2
	mul bx
	mov edi, eax
	mov ah, cl
.1:
        lodsb
        test al, al
        jz .2
        cmp al, 0x0A    ;是回车吗
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
	pop ecx
        pop ebx
        pop ebp
        ret

;void clean(int top, int left, int bottom, int right)
_clean:
	push ebp
	mov ebp, esp
	;push ebx
	;push ecx
	push esi
	push edi

	;栈内顺序
	;top
	;left
	;bottom
	;right
	mov eax, [ebp+8]	;上
	mov ecx, [ebp+12]	;左
	;计算清空显存开始位置
	mov bx, 80
	mul bx
	and eax,0xFFFF
	shl edx,0x10
	add eax, edx
		
	add eax,ecx
	mov bx,2
	mul bx
	mov edi, eax
	;计算结束地址
	mov eax, [ebp+16]	;下
	mov ecx, [ebp+20]	;右
	mov bx, 80
	mul bx
	and eax, 0xFFFF
	shl edx, 0x10
	add eax,edx	
	add eax, ecx
	mov bx, 2
	mul bx
.start:
	cmp edi, eax
	jnl .end
	mov edx,0x00
	mov [gs:edi], edx
	add edi, 2
	jmp .start
.end:
	pop edi
	pop esi
	;pop ebx
	pop ebp
	ret	


