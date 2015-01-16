;定义各个程序使用的内存地址
BaseOfBootStack		equ	0x7c00		;boot.bin使用的栈基址
BaseOfLoaderStack	equ	0x0100		;loader.bin使用的栈基址

;loader.bin被加载到的基地址和偏移地址
BaseOfLoader		equ	0x9000
OffsetOfLoader		equ	0x0100
BaseOfLoaderPhyAddr	equ	BaseOfLoader * 0x0010
;kernel.bin被加载到的基址和偏移地址
BaseOfKernel		equ	0x8000
OffsetOfKernel		equ	0x0000


;32保护模式下
PageDirBase		equ	0x100000	;页目录开始地址：1M
PageTblBase		equ	0x101000	;页表开始地址：1M+4K
