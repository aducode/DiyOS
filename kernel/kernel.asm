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
global _restart
_start:
	;将栈移到内核内存空间中
	mov esp, StackTop;
	sgdt [gdt_ptr]	;给全局变量赋值
	call head	;head中进行初始化gdt idt的工作
	lgdt [gdt_ptr]	;使用新的gdt
	
	lidt [idt_ptr]	;	
	jmp SELECTOR_KERNEL_CS:csinit
csinit:	;这个跳转指令强制使用刚刚初始化的结构
	push 0
	popfd	;清空eflag寄存器的值	
	;ud2	;让cpu产生undefined error 测试异常向量是否设置正确
	;开中断
	;sti	;现在还不能开中断，要kmain中初始化进程表之后才能开中断,kmain中调用restart，restart中iret后自动开中断，然后就可以切换到进程中了
	;hlt
	;jmp $-1
	;跳转到kernel主函数
	push kmain
	ret

;save 在中断发生时，保存当前进程寄存器的值
_save:
	;中断发生时，是从ring1以上跳到ring0，首先cpu会自动从TSS中的esp0中取出esp的值（已经在进程第一次启动调用restart时，被设置成该进程的进程表栈顶），然后中断时cpu会将cs eip等值入栈。
	;call save时，会将save的返回地址也压栈，（保存在进程表中的retaddr里面）
	;保存原寄存器的值
	pushad
	push ds
	push es
	push fs
	push gs
	;这里开始不能再使用push pop了，因为当前esp指向进程表里的某个位置，在进行栈操作会破坏进程表
	mov esi, edx	;保存edx （系统调用参数)
	;将ds es也指向ss
	mov dx, ss
	mov ds, dx
	mov es, dx
	mov fs, dx
	mov edx, esi	;恢复edx

	mov esi, esp            ;esp已经压栈压到进成表项的最低地址了，所以此时esi就指向了进程表项开始位置
	
	inc dword[k_reenter]	;中断+1
	cmp dword[k_reenter], 0	;==0表示已经在中断中了
	jne .1			
	mov esp, StackTop	;将esp从进程表切换到内核栈
	;下面可以放心使用push pop了，因为esp已经在内核栈中了
	push _restart			;将retaddr执行完后的返回地址压栈
	jmp [esi+RETADR - P_STACKBASE]	;返回到进程表项中的retaddr指向地址,起始retaddr就是call save的下一条指令
.1:
	;说明已经在内核中了，不需要从新设置进程的ldt和tss esp0等值
	push _restart_reenter
	jmp [esi+RETADR - P_STACKBASE]



;从内核中恢复用户线程上下
_restart:
;	sti
;	ret
	mov esp, [p_proc_ready]		;设置esp指向占有cpu时间片的进程表项
	lldt [esp+P_LDT_SEL]		;加载进程ldt
	lea eax, [esp+P_STACKTOP]
	mov dword[g_tss+TSS3_S_SP0], eax	;设置下次中断发生，从低特权跳到高特权级别时esp的位置到进程表项的栈底位置
_restart_reenter:
	dec dword [k_reenter]		;中断返回前k_reenter 减1
	pop gs				
	pop fs
	pop es
	pop ds
	popad				;恢复各个寄存器的值
	add esp, 4			;esp跳过进程表中retaddr
	iretd				;中断返回，cs eip esp等寄存器会从进程表栈中恢复，继续之前的进程执行


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

;硬件中断
; 中断和异常 -- 硬件中断
global  _hwint00
global  _hwint01
global  _hwint02
global  _hwint03
global  _hwint04
global  _hwint05
global  _hwint06
global  _hwint07
global  _hwint08
global  _hwint09
global  _hwint10
global  _hwint11
global  _hwint12
global  _hwint13
global  _hwint14
global  _hwint15

; ---------------------------------
%macro  hwint_master    1
	;中断发生时，从ring1跳入ring0，首先esp的值会被变成tss中esp0的值，也就是上次中断返回之前设置成的进程表栈底
	;然后原进程的cs eip esp等入栈
	;然后call save，会把save返回地址入栈
	call _save		;save保存进程上下文，并把esp从进程表转到内核栈中，并将restart或restart_reenter入栈
	in al, INT_M_CTLMASK
	or al, (1<<%1)
	out INT_M_CTLMASK, al	;屏蔽同种类的中断
	mov al, EOI	
	out INT_M_CTL, al	;置EOI
	sti			;cpu在响应中断过程中会关中断，这句之后就允许响应新中断（其他种类的中断）
	push %1	
	call [irq_handler_table+4*%1] 
	pop ecx
	cli			;关中断, 后面iret后会自动开中断的
	in al, INT_M_CTLMASK
	and al, ~(1<<%1)
	out INT_M_CTLMASK, al	;恢复当前中断
	ret			;ret会返回到栈顶的地址，而栈顶已经在save中被push成restart 或 restart_reenter了，所以ret之后会jmp到restart，来恢复进程上线文，并从中断返回
%endmacro
; ---------------------------------

ALIGN   16
_hwint00:                ; Interrupt routine for irq 0 (the clock).
        hwint_master    0

ALIGN   16
_hwint01:                ; Interrupt routine for irq 1 (keyboard)
        hwint_master    1

ALIGN   16
_hwint02:                ; Interrupt routine for irq 2 (cascade!)
        hwint_master    2

ALIGN   16
_hwint03:                ; Interrupt routine for irq 3 (second serial)
        hwint_master    3

ALIGN   16
_hwint04:                ; Interrupt routine for irq 4 (first serial)
        hwint_master    4

ALIGN   16
_hwint05:                ; Interrupt routine for irq 5 (XT winchester)
        hwint_master    5

ALIGN   16
_hwint06:                ; Interrupt routine for irq 6 (floppy)
        hwint_master    6

ALIGN   16
_hwint07:                ; Interrupt routine for irq 7 (printer)
        hwint_master    7

; ---------------------------------
%macro  hwint_slave     1
        push    %1
        call    [irq_handler_table+4*%1]
        add     esp, 4
%endmacro
; ---------------------------------

ALIGN   16
_hwint08:                ; Interrupt routine for irq 8 (realtime clock).
        hwint_slave     8

ALIGN   16
_hwint09:                ; Interrupt routine for irq 9 (irq 2 redirected)
        hwint_slave     9

ALIGN   16
_hwint10:                ; Interrupt routine for irq 10
        hwint_slave     10

ALIGN   16
_hwint11:                ; Interrupt routine for irq 11
        hwint_slave     11

ALIGN   16
_hwint12:                ; Interrupt routine for irq 12
        hwint_slave     12

ALIGN   16
_hwint13:                ; Interrupt routine for irq 13 (FPU exception)
        hwint_slave     13

ALIGN   16
_hwint14:                ; Interrupt routine for irq 14 (AT winchester)
        hwint_slave     14

ALIGN   16
_hwint15:                ; Interrupt routine for irq 15
        hwint_slave     15

