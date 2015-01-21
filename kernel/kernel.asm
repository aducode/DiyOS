;导入
;宏
;disp_str_color		equ	 0x0F
%include	"kernel.inc"
extern kmain
;[section .s32] 当设置这个section时，ld使用-Ttext选项指定的entry point不准确
;[section .text] 改成这个或者彻底注释掉就可以使用-Ttext
[section .s32] ;也可以用 ld 的 -e 选项替代-Ttext
global _start
global _lgdt		;void _lgdt(struct descriptor_table gdt_ptr);
global _hlt
global disp_str
global clean
_start:
	push kmain
	ret
	;jmp $

_lgdt:
	mov eax, [esp+4]	;limit_of_gdt
	and eax, 0xFF
	mov word[gdt_limit], ax
	mov eax, [esp+8]; base_of_gdt
	mov dword[gdt_base],eax
	lgdt [gdt_ptr]
	ret
_hlt:
	hlt
	ret

;函数：void disp_str(char * str, int row, int column)
;loader.bin中已经将显存基址设置在gs寄存器中
disp_str:
	push ebp
	mov ebp, esp
;	push ebx
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
	;计算显存地址
	mov bx,80
	mul bx
	add eax,edx
	mov bx, 2
	mul bx
	mov edi, eax
	mov ah, STRING_COLOR
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
;        pop ebx
        pop ebp
        ret

;void clean(int top, int left, int bottom, int right)
clean:
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

[section .data]
;
gdt_ptr:
gdt_limit:	dw	0
gdt_base:	dd	0
