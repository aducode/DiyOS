定义系统级进程主体
* tty.c		tty进程相关函数
* ticks.c	ticks进程，测试进程通讯使用，用于get_ticks函数
* hd.c		硬盘驱动
* test.c	测试进程调度

tty.c进程在每个cpu时间片中依次调用do_tty_read （调用keyboard_read从scan code buffer读取scan code解析成key 然后再放入tty的字串缓冲区）和 do_tty_write (用console的out_char反函数将tty字符缓冲区内的字符设置到console对应显存位置，来显示字符串)
