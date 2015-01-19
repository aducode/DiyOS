#编译kernel
.PHONY:clean all disasm
KERNEL=kernel
KERNEL_ENTRY_POINT=0x0400
KERNEL_ENTRY_OFFSET=0x0400


CC=gcc
CFLAGS=-I include/ -c -fno-builtin
ASM=nasm
ASMFLAGS=-f elf
LD=ld
LDFLAGS=-s -Ttext $(KERNEL_ENTRY_POINT)

DASM=ndisasm
DASMFLAGS=-u -o $(KERNEL_ENTRY_POINT) -e $(KERNEL_ENTRY_OFFSET)
DASMOUTPUT=$(KERNEL).bin.asm

OBJECTS=$(KERNEL).o main.o

all:$(KERNEL).bin

$(KERNEL).bin:$(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $(KERNEL).bin

main.o:main.c
	$(CC) $(CFLAGS) main.c -o main.o
	
$(KERNEL).o:$(KERNEL).asm
	$(ASM) $(ASMFLAGS) $(KERNEL).asm -o $(KERNEL).o
disasm:$(KERNEL).bin
	$(DASM) $(DASMFLAGS) $(KERNEL).bin > $(DASMOUTPUT)
clean:
	-@rm -rf *.bin *.o $(DASMOUTPUT)
