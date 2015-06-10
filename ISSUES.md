###ISSUES
file: kernel/include/type.h
line: 32
去掉这行注释，程序就可以正常运行
加上这行注释，kernel/task/init.c 中打开stdin的时候就会卡主不动，难道跟代码大小有关（实在想不通为什么会出现这种情况)
