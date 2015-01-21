;---------------------------------------------------------------------------
;全局的函数
extern head		;用来初始化gdt idt等的c函数
extern kmain		;c语言内核主函数
;全局变量
extern gdt_ptr		;全局的gdt_ptr





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

