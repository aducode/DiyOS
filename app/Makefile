ASM=nasm
ASMFLAGS=-I ../kernel/include -f elf
CC=gcc
CFLAGS=-I ../kernel/include -c -fno-builtin -Wall
LD=ld
LDFLAGS=-s -Ttext 0x1000

.PHONY:clean  all
all:clean echo

start.o:start.asm
	$(ASM) $(ASMFLAGS) start.asm -o start.o
#diycrt 操作系统的静态库
diycrt.a:
	make lib -C ../kernel
clean:
	-@rm -rf target
	-@rm -rf *.a *.o

echo.o:echo.c
	$(CC) $(CFLAGS)	-o echo.o echo.c
echo:echo.o start.o diycrt.a
	-@mkdir target
	$(LD) $(LDFLAGS) -o target/echo echo.o start.o diycrt.a
