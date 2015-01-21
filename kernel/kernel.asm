;导入宏
;disp_str_color		equ	 0x0F
%include	"kernel.inc"
[section .bss]	;栈空间
StackeSpace:	resb	2*1024
StackTop:	;栈顶


;[section .s32] 当设置这个section时，ld使用-Ttext选项指定的entry point不准确
;[section .text] 改成这个或者彻底注释掉就可以使用-Ttext
;[section .s32] ;也可以用 ld 的 -e 选项替代-Ttext<---这种做法也是错的，程序入口地址会被指定为0x0400，但这时虚拟地址的偏移，真正的segment还是高位地址
;最终还是指定一个标准的section .text
[section .text]
global _start
global _memcpy		;void _memcpy(void * dest, void * src, int length);
global _hlt
global _disp_str
global _clean
_start:
	;将栈移到内核内存空间中
	mov esp, StackTop;
	sgdt [gdt_ptr]	;给全局变量赋值
	call head
	lgdt [gdt_ptr]	;使用新的gdt
	
	jmp CodeSelector:csinit
csinit:	;这个跳转指令强制使用刚刚初始化的结构
	push 0
	popfd	;清空eflag寄存器的值	
	;跳转到kernel主函数
	push kmain
	ret



;-----------------------------------------------------------------
;定义一些函数供c语言使用
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
_hlt:
	hlt
	ret

;函数：void disp_str(char * str, int row, int column)
;loader.bin中已经将显存基址设置在gs寄存器中
_disp_str:
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

;[section .data]
;
;gdt_ptr:
;gdt_limit:	dw	0
;gdt_base:	dd	0
