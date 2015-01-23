;---------------------------------------------------------------------------
;全局的函数
extern head		;用来初始化gdt idt等的c函数
extern kmain		;c语言内核主函数
extern exception_handler	;异常处理函数
;extern irq_handler		;硬件中断处理函数
;全局变量
extern gdt_ptr		;全局的gdt_ptr
extern idt_ptr		;全局中断向量表指针

extern p_proc_ready	;获取cpu时间片的进程表指针

extern k_reenter	;

extern g_tss

extern irq_handler_table	;全局硬件中断处理函数

MAX_GDT_ITEMS	equ	128
LIMIT_OF_GDT	equ	(MAX_GDT_ITEMS - 1) * 8

;color
STRING_COLOR	equ	0x0F

;
;----------------------------------------------------------------------------
; 选择子类型值说明
; 其中:
;       SA_  : Selector Attribute

SA_RPL0         EQU     0       ; ┓
SA_RPL1         EQU     1       ; ┣ RPL
SA_RPL2         EQU     2       ; ┃
SA_RPL3         EQU     3       ; ┛

SA_TIG          EQU     0       ; ┓TI
SA_TIL          EQU     4       ; ┛
;-------------------------------------------------------------------------
;Selector
SELECTOR_FLAT_C	equ	1<<3
SELECTOR_TSS	equ	4<<3
;CodeSelector	equ	1<<3
;DataSelector	equ	2<<3
;VideoSelector	equ	3<<3 + SA_RPL3
;TSSSeclector	equ	4<<3	;TSS的选择子
SELECTOR_KERNEL_CS equ	SELECTOR_FLAT_C
;8259A
INT_M_CTL	equ	0x20
INT_M_CTLMASK	equ	0x21
INT_S_CTL	equ	0xA0
INT_S_CTLMASK	equ	0xA1

EOI		equ	0x20


;进程表中的栈相关宏
P_STACKBASE	equ	0x00
GSREG		equ	P_STACKBASE	+	4
FSREG		equ	GSREG		+	4
ESREG		equ	FSREG		+	4
DSREG		equ	ESREG		+	4
EDIREG		equ	DSREG		+	4
ESIREG		equ	EDIREG		+	4
EBPREG		equ	ESIREG		+	4
KERNELESPREG	equ	EBPREG		+	4
EBXREG		equ	KERNELESPREG	+	4
EDXREG		equ	EBXREG		+	4
ECXREG		equ	EDXREG		+	4
EAXREG		equ	ECXREG		+	4
RETADR		equ	EAXREG		+	4
EIPREG		equ	RETADR		+	4
CSREG		equ	EIPREG		+	4
EFLAGSREG	equ	CSREG		+	4
ESPREG		equ	EFLAGSREG	+	4
SSREG		equ	ESPREG		+	4
P_STACKTOP	equ	SSREG		+	4

P_LDT_SEL	equ	P_STACKTOP
P_LDT		equ	P_LDT_SEL	+	4


;TSS
TSS3_S_SP0	equ	4
