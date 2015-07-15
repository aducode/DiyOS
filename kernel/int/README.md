异常&硬件中断响应函数,负责与硬件进行交互
* clock.c	时钟中断
* keyboard.c	键盘中断
* console.c	输出控制台
* interrupt.c	异常中断处理函数，和默认的硬件中断处理函数 
* console.c	控制台，直接处理显存
* hd.c		硬盘中断
* floppy.c  软盘中断(in dev)

keyboard.c中维护一个键盘扫描码(scan code)缓存，与tty进程交互，tty在每个cpu时间片调用keyboard.c的keyboard_read方法解析scan code成系统定义的key，然后再给对应console输出到屏幕（因为键盘中断相对于时钟中断发生的频率要小很多，所以这个scan code buffer只设置成32byte）


键盘scan code参考：
http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
