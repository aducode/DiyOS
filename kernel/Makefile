#编译kernel
.PHONY:clean lib all disasm
KERNEL=kernel
KERNEL_ENTRY_POINT=0x0400
KERNEL_ENTRY_OFFSET=0x0400


CC=gcc
#使用clang编译的时候，不定参数会出现问题
#CC=clang
CFLAGS=-I include/ -I include/sys/ -I include/common/ -c -fno-builtin
ASM=nasm
ASMFLAGS=-I include/ -I include/sys/ -f elf
LD=ld
LDFLAGS=-s -Ttext $(KERNEL_ENTRY_POINT)
AR=ar
ARFLAGS=rcs
AROUTFILE=../app/diycrt.a
ARINFILE=syscall.o string.o stdio.o assert.o systicks.o getpid.o fork.o exit.o wait.o ipc.o
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

OBJECTS=$(KERNEL).o head.o global.o main.o interrupt.o klib.o string.o stdio.o stat.o mount.o systicks.o getpid.o fork.o exit.o wait.o tar.o assert.o clock.o keyboard.o hd.o floppy.o console.o proc.o ipc.o syscall.o tty.o hddriver.o floppydriver.o sys.o fs.o tortoise.o fat12.o mm.o init.o 
#map.o
#OBJECTS=$(KERNEL).o main.o global.o head.o klib.o  interrupt.o string.o stdio.o clock.o keyboard.o console.o proc.o  syscall.o tty.o test.o

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
stdio.o:lib/stdio.c
	$(CC) $(CFLAGS) lib/stdio.c -o stdio.o
stat.o:lib/stat.c
	$(CC) $(CFLAGS) lib/stat.c -o stat.o
mount.o:lib/mount.c
	$(CC) $(CFLAGS) lib/mount.c -o mount.o
tar.o:lib/tar.o
	$(CC) $(CFLAGS) lib/tar.c -o tar.o
assert.o:lib/assert.c
	$(CC) $(CFLAGS) lib/assert.c -o assert.o
systicks.o:lib/systicks.c
	$(CC) $(CFLAGS) lib/systicks.c -o systicks.o
getpid.o:lib/getpid.c
	$(CC) $(CFLAGS) lib/getpid.c -o getpid.o
fork.o:lib/fork.c
	$(CC) $(CFLAGS) lib/fork.c -o fork.o
exit.o:lib/exit.c
	$(CC) $(CFLAGS) lib/exit.c -o exit.o
wait.o:lib/wait.c
	$(CC) $(CFLAGS) lib/wait.c -o wait.o
ipc.o:lib/ipc.c
	$(CC) $(CFLAGS) lib/ipc.c -o ipc.o

################################# SYS INT ###################################
interrupt.o:int/interrupt.c
	$(CC) $(CFLAGS) int/interrupt.c -o interrupt.o
clock.o:int/clock.c
	$(CC) $(CFLAGS) int/clock.c -o clock.o
keyboard.o:int/keyboard.c
	$(CC) $(CFLAGS) int/keyboard.c -o keyboard.o
console.o:int/console.c
	$(CC) $(CFLAGS) int/console.c -o console.o
hd.o:int/hd.c
	$(CC) $(CFLAGS) int/hd.c -o hd.o
floppy.o:int/floppy.c
	$(CC) $(CFLAGS) int/floppy.c -o floppy.o


########################### TASKS ##########################################
tty.o:task/tty/tty.c
	$(CC) $(CFLAGS) task/tty/tty.c -o tty.o
hddriver.o:task/driver/hddriver.c
	$(CC) $(CFLAGS) task/driver/hddriver.c -o hddriver.o
floppydriver.o:task/driver/floppydriver.c
	$(CC) $(CFLAGS) task/driver/floppydriver.c -o floppydriver.o
sys.o:task/sys/sys.c
	$(CC) $(CFLAGS) task/sys/sys.c -o sys.o
fs.o:task/fs/fs.c
	$(CC) $(CFLAGS) task/fs/fs.c -o fs.o
tortoise.o:task/fs/tortoise.c
	$(CC) $(CFLAGS) task/fs/tortoise.c -o tortoise.o
fat12.o:task/fs/fat12.c
	$(CC) $(CFLAGS) task/fs/fat12.c -o fat12.o
mm.o:task/mm/mm.c
	$(CC) $(CFLAGS) task/mm/mm.c -o mm.o
init.o:task/init.c
	$(CC) $(CFLAGS) task/init.c -o init.o
	
########################### common #########################################
#map.o:common/map.c
#	$(CC) $(CFLAGS) common/map.c -o map.o
	
########################### PHONY ##########################################
lib:$(ARINFILE)
	$(AR) $(ARFLAGS) $(AROUTFILE) $(ARINFILE)
disasm:$(KERNEL).bin
	$(DASM) $(DASMFLAGS) $(KERNEL).bin > $(DASMOUTPUT)
clean:
	-@rm -rf *.bin *.o $(DASMOUTPUT)
