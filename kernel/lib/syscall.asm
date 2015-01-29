;系统调用API
;系统调用中断号
INT_VECTOR_SYS_CALL	equ	0x80	;与linux保持一致


;系统调用函数表
SYS_CALL_GET_TICKS	equ	0

global get_ticks
bits 32
[section .text]
get_ticks:
	mov eax, SYS_CALL_GET_TICKS
	int INT_VECTOR_SYS_CALL
	ret
