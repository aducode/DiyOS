#编译kernel
.PHONY:clean all
KERNEL=kernel

ASM=nasm -f elf

all:$(KERNEL).bin

$(KERNEL).bin:$(KERNEL).asm
	$(ASM) $(KERNEL).asm -o $(KERNEL).bin

clean:
	-@rm -rf *.bin
