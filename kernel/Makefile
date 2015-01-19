#编译kernel
.PHONY:clean all
KERNEL=kernel
KERNEL_ENTRY_POINT=0x0400

ASM=nasm -f elf
LD=ld -s -Ttext $(KERNEL_ENTRY_POINT)

all:$(KERNEL).bin

$(KERNEL).bin:$(KERNEL).o
	$(LD) $(KERNEL).o -o $(KERNEL).bin

$(KERNEL).o:$(KERNEL).asm
	$(ASM) $(KERNEL).asm -o $(KERNEL).o

clean:
	-@rm -rf *.bin *.o
