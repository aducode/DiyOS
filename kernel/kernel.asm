;导入
extern kmain
global _start
global _hlt
_start:
	push kmain
	ret
	;jmp $
_hlt:
	hlt
	jmp $

