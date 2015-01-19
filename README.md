#自己动手自作一个操作系统
一些说明
* 参考《Orange's一个操作系统的实现》、《30天自制操作系统》。
* 这个操作系统目前只能通过软盘引导，同时为了方便编程，使用fat12格式化的软盘（fat文件系统头部信息已经写入boot.asm中），使用mount -o loop,rw a.img /xxx可以挂在，并且将其他程序写入软盘；相对的，linux 加载0扇区bootsec后，接着用bootsec加载1-4扇区的setup程序，setup继续引导剩余的全部扇区到内存，但是这样写入组织好的多个文件到软盘操作起来不是很方便，所以这里借用fat12格式的文件系统。
* 为了方便使用c语言，系统内核和未来的应用程序均使用ELF格式的可执行文件，这样就可以在Linux中编写程序。
