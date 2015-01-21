;---------------------------------------------------------------------------
;全局的函数
extern head		;用来初始化gdt idt等的c函数
extern kmain		;c语言内核主函数
extern exception_handler	;异常处理函数
extern irq_handler		;硬件中断处理函数
;全局变量
extern gdt_ptr		;全局的gdt_ptr
extern idt_ptr		;全局中断向量表指针




MAX_GDT_ITEMS	equ	32
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
CodeSelector	equ	1<<3
DataSelector	equ	2<<3
VideoSelector	equ	3<<3 + SA_RPL3
;8259A
INT_M_CTL		equ	0x20
INT_M_CTLMASK		equ	0x21
INT_S_CTL		equ	0xA0
INT_S_CTLMASK		equ	0xA1

EOI			equ	0x20
