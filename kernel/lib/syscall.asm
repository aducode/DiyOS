;系统调用API
;系统调用中断号
INT_VECTOR_SYS_CALL	equ	0x80	;与linux保持一致


;系统调用函数表
SYS_CALL_WRITE	equ	0

global write
bits 32
[section .text]
write:
	mov eax, SYS_CALL_WRITE
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	ret
