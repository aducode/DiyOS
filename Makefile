.PHONY:clean mkimg run die all
BOOT=boot
FLOPPY=a.img
LOGS = copy.txt bochsout.txt

all:clean $(FLOPPY) run

run:
	bochs -qf bochsrc

mkimg:
	bximage -fd -size=1.44 -q a.img


$(FLOPPY):mkimg $(BOOT).bin
	dd if=$(BOOT).bin of=$(FLOPPY) bs=512 count=1 conv=notrunc

$(BOOT).bin:$(BOOT).asm
	nasm $(BOOT).asm -o $(BOOT).bin
clean:
	-rm $(FLOPPY) $(BOOT).bin $(LOGS)

die:
	@echo "kill bochs"
	@kill -9 `ps aux|grep bochs|grep -v 'grep'|awk '{print $$2}'` 
	@echo "killed"
