自己动手自作一个操作系统
一些说明：

*这个操作系统目前只能通过软盘引导，同时为了方便编程，使用fat12格式化的软盘（fat文件系统头部信息已经写入boot.asm中），使用mount -o loop,rw a.img /xxx可以挂在，并且将其他程序写入软盘；相对的，linux 加载0扇区bootsec后，接着用bootsec加载1-4扇区的setup程序，setup继续引导剩余的全部扇区到内存，但是这样写入组织好的多个文件到软盘操作起来不是很方便，所以这里借用fat12格式的文件系统。

*

目录结构：
1.boot/	
	1.1. boot.asm boot.bin的源码，boot.bin是软盘boot扇区的代码
	1.2. loader.asm  loader.bin源码，boot.bin加载loader，loader用于真正加载操作系统内核到内存
	1.3. boot/include/	被引用的目录
		1.3.1.fat12hdr.in	供loader.asm boot.asm include使用的，里面定义了软盘的一些信息和常量
		1.3.2.lib16.inc	定义了一些共用汇编函数 供boot loader使用，用宏_BOOT_USE_控制 实模式下使用
		1.3.3.memmap.inc	定义了内存地址分布
		1.3.4.pm.inc	定义了GDT IDT 相关的宏
		1.3.5. lib32.inc	定义了一些函数，保护模式下使用
	1.4.Makefile	make：编译boot.bin loader.bin
2.config/
	2.1.bochsrc	bochs的配置文件 bochs -qf config/bochsrc
3.test/	一些之前用来测试的文件
4.Makefile	make file文件，用于制作软盘镜像，启动bochs

