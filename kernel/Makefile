#编译kernel
.PHONY:clean all disasm
KERNEL=kernel
KERNEL_ENTRY_POINT=0x0400
KERNEL_ENTRY_OFFSET=0x0400


CC=gcc
CFLAGS=-I include/ -I include/sys/ -c -fno-builtin
ASM=nasm
ASMFLAGS=-I include/ -I include/sys/ -f elf
LD=ld
LDFLAGS=-s -Ttext $(KERNEL_ENTRY_POINT)
#LDFLAGS=-s -e $(KERNEL_ENTRY_POINT)
# 用-e替代 -Ttext链接脚本
# 由于在kernel.asm中显示的写上[section .s32],导致-Ttext设置后真正的Entry不相符，
# fix 方案：
# 1.  发现去掉[section .s32] 或改成 [section .text] 
# 2.  使用-e 替代 -Ttext
# 参考资料：
# ld有多种方法设置进程入口地址, 按一下顺序: (编号越前, 优先级越高)
#1, ld命令行的-e选项
#2, 连接脚本的ENTRY(SYMBOL)命令
#3, 如果定义了_start符号, 使用_start符号值
#4, 如果存在.text section, 使用.text section的第一字节的位置值
#5, 使用值0


DASM=ndisasm
DASMFLAGS=-u -o $(KERNEL_ENTRY_POINT) -e $(KERNEL_ENTRY_OFFSET)
DASMOUTPUT=$(KERNEL).bin.asm

OBJECTS=$(KERNEL).o main.o global.o head.o klib.o  interrupt.o string.o clock.o keyboard.o proc.o  syscall.o tty.o test.o

all:$(KERNEL).bin

$(KERNEL).bin:$(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $(KERNEL).bin

#################################kernel main ######################################
$(KERNEL).o:$(KERNEL).asm
	$(ASM) $(ASMFLAGS) $(KERNEL).asm -o $(KERNEL).o
klib.o:lib/klib.asm
	$(ASM) $(ASMFLAGS) lib/klib.asm -o klib.o
head.o:head.c
	$(CC) $(CFLAGS) head.c -o head.o
global.o:global.c
	$(CC) $(CFLAGS) global.c -o global.o
main.o:main.c
	$(CC) $(CFLAGS) main.c -o main.o
proc.o:proc.c
	$(CC) $(CFLAGS) proc.c -o proc.o


############################# lib ############################################
syscall.o:lib/syscall.asm
	$(ASM) $(ASMFLAGS) lib/syscall.asm -o syscall.o
string.o:lib/string.c
	$(CC) $(CFLAGS) lib/string.c -o string.o


################################# SYS INT ###################################
interrupt.o:int/interrupt.c
	$(CC) $(CFLAGS) int/interrupt.c -o interrupt.o
clock.o:int/clock.c
	$(CC) $(CFLAGS) int/clock.c -o clock.o
keyboard.o:int/keyboard.c
	$(CC) $(CFLAGS) int/keyboard.c -o keyboard.o


########################### TASKS ##########################################
tty.o:task/tty.c
	$(CC) $(CFLAGS) task/tty.c -o tty.o
test.o:task/test.c
	$(CC) $(CFLAGS) task/test.c -o test.o
	
########################### PHONY ##########################################
disasm:$(KERNEL).bin
	$(DASM) $(DASMFLAGS) $(KERNEL).bin > $(DASMOUTPUT)
clean:
	-@rm -rf *.bin *.o $(DASMOUTPUT)
