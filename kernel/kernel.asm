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
_start:
	;将栈移到内核内存空间中
	mov esp, StackTop;
	sgdt [gdt_ptr]	;给全局变量赋值
	call head
	lgdt [gdt_ptr]	;使用新的gdt
	
	lidt [idt_ptr]	;	
	jmp CodeSelector:csinit
csinit:	;这个跳转指令强制使用刚刚初始化的结构
	push 0
	popfd	;清空eflag寄存器的值	
	;ud2	;让cpu产生undefined error 测试异常向量是否设置正确
	;跳转到kernel主函数
	push kmain
	ret



; 中断和异常 -- 异常
global	_divide_error
global	_single_step_exception
global	_nmi
global	_breakpoint_exception
global	_overflow
global	_bounds_check
global	_inval_opcode
global	_copr_not_available
global	_double_fault
global	_copr_seg_overrun
global	_inval_tss
global	_segment_not_present
global	_stack_exception
global	_general_protection
global	_page_fault
global	_copr_error


_divide_error:
	push	0xFFFFFFFF	; no err code
	push	0		; vector_no	= 0
	jmp	exception
_single_step_exception:
	push	0xFFFFFFFF	; no err code
	push	1		; vector_no	= 1
	jmp	exception
_nmi:
	push	0xFFFFFFFF	; no err code
	push	2		; vector_no	= 2
	jmp	exception
_breakpoint_exception:
	push	0xFFFFFFFF	; no err code
	push	3		; vector_no	= 3
	jmp	exception
_overflow:
	push	0xFFFFFFFF	; no err code
	push	4		; vector_no	= 4
	jmp	exception
_bounds_check:
	push	0xFFFFFFFF	; no err code
	push	5		; vector_no	= 5
	jmp	exception
_inval_opcode:
	push	0xFFFFFFFF	; no err code
	push	6		; vector_no	= 6
	jmp	exception
_copr_not_available:
	push	0xFFFFFFFF	; no err code
	push	7		; vector_no	= 7
	jmp	exception
_double_fault:
	push	8		; vector_no	= 8
	jmp	exception
_copr_seg_overrun:
	push	0xFFFFFFFF	; no err code
	push	9		; vector_no	= 9
	jmp	exception
_inval_tss:
	push	10		; vector_no	= A
	jmp	exception
_segment_not_present:
	push	11		; vector_no	= B
	jmp	exception
_stack_exception:
	push	12		; vector_no	= C
	jmp	exception
_general_protection:
	push	13		; vector_no	= D
	jmp	exception
_page_fault:
	push	14		; vector_no	= E
	jmp	exception
_copr_error:
	push	0xFFFFFFFF	; no err code
	push	16		; vector_no	= 10h
	jmp	exception

exception:
	call	exception_handler
	add	esp, 4*2	; 让栈顶指向 EIP，堆栈中从顶向下依次是：EIP、CS、EFLAGS
	hlt
