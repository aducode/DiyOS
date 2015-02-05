;系统调用API
;系统调用中断号
INT_VECTOR_SYS_CALL	equ	0x80	;与linux保持一致


;系统调用函数表
SYS_CALL_SENDREC	equ	0	;系统作成微内核，未来只保留一个系统调用，用于进程间通信
SYS_CALL_PRINTK		equ	1
SYS_CALL_WRITE		equ	2

global sendrec
global printk
global write
bits 32
[section .text]
sendrec:
	mov eax, SYS_CALL_SENDREC
	mov ebx, [esp + 4]	;function 区分是send还是receive
	mov ecx, [esp + 8]	;消息地址
	mov edx, [esp +12]	;消息体
	int INT_VECTOR_SYS_CALL
	ret
printk:
	mov eax, SYS_CALL_PRINTK
	mov edx, [esp + 4]	;printk(char * buffer);
	int INT_VECTOR_SYS_CALL
	ret	
write:
	mov eax, SYS_CALL_WRITE
	mov ecx, [esp + 4]
	mov edx, [esp + 8]
	int INT_VECTOR_SYS_CALL
	ret
