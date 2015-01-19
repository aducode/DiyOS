#编译kernel
.PHONY:clean all
KERNEL=kernel
KERNEL_ENTRY_POINT=0x0400


CC=gcc
CFLAGS=-I include/ -c -fno-builtin
ASM=nasm
ASMFLAGS=-f elf
LD=ld
LDFLAGS=-s -Ttext $(KERNEL_ENTRY_POINT)

OBJECTS=$(KERNEL).o main.o

all:$(KERNEL).bin

$(KERNEL).bin:$(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $(KERNEL).bin

main.o:main.c
	$(CC) $(CFLAGS) main.c -o main.o
	
$(KERNEL).o:$(KERNEL).asm
	$(ASM) $(ASMFLAGS) $(KERNEL).asm -o $(KERNEL).o

clean:
	-@rm -rf *.bin *.o
