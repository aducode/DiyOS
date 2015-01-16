自己动手自作一个操作系统
一些说明：

*这个操作系统目前只能通过软盘引导，同时为了方便编程，使用fat12格式化的软盘（fat文件系统头部信息已经写入boot.asm中），使用mount -o loop,rw a.img /xxx可以挂在，并且将其他程序写入软盘；相对的，linux 加载0扇区bootsec后，接着用bootsec加载1-4扇区的setup程序，setup继续引导剩余的全部扇区到内存，但是这样写入组织好的多个文件到软盘操作起来不是很方便，所以这里借用fat12格式的文件系统。

*

目录结构：
1. boot.asm boot.bin的源码，boot.bin是软盘boot扇区的代码
2. loader.asm  loader.bin源码，boot.bin加载loader，loader用于真正加载操作系统内核到内存
3.fat12hdr.in	供loader.asm boot.asm include使用的，里面定义了软盘的一些信息和常量
4.lib.inc	定义了一些共用的函数
5.memmap.inc	定义了内存地址分布
6.pm.inc	定义了GDT IDT 相关的宏
7.Makefile	make：default run the bochs
8.bochsrc	bochs的配置文件 bochs -qf bochsrc
