#编译boot.asm loader.asm
.PHONY:clean all

BOOT=boot
LOADER=loader
KERNEL=kernel


ASM = nasm -I include/

all:$(BOOT).bin $(LOADER).bin $(KERNEL).bin

$(BOOT).bin:$(BOOT).asm
	$(ASM) $(BOOT).asm -o $(BOOT).bin
$(LOADER).bin:$(LOADER).asm
	$(ASM) $(LOADER).asm -o $(LOADER).bin
$(KERNEL).bin:$(KERNEL).asm
	$(ASM) $(KERNEL).asm -o $(KERNEL).bin
clean:
	-@rm  *.bin
