定义系统级进程主体,系统微内核架构，每个系统功能都作为一个进程，使用消息与其他进程通信
* tty/tty.c		tty进程相关函数
* sys/sys.c	sys进程，测试进程通讯使用，用于get_ticks函数
* driver/hddriver.c	硬盘驱动，main loop中主要调用int/hd.c对硬盘操作
* driver/floppydriver.c 软盘驱动
* fs/fs.c		文件系统进程，对外暴露文件接口
* fs/fat12.c fat12文件系统抽象层
* fs/tortoise.c tortoise文件系统抽象层
* init.c 用户进程根，用于fork子用户进程

tty.c进程在每个cpu时间片中依次调用do_tty_read （调用keyboard_read从scan code buffer读取scan code解析成key 然后再放入tty的字串缓冲区）和 do_tty_write (用console的out_char反函数将tty字符缓冲区内的字符设置到console对应显存位置，来显示字符串)
