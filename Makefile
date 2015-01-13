.PHONY:clean img run die all
BOOT=boot
FLOPPY=a.img
LOGS = *.txt *.log

all:clean $(FLOPPY) run

run:
	bochs -qf bochsrc

img:
	bximage -fd -size=1.44 -q a.img


$(FLOPPY):img $(BOOT).bin
	dd if=$(BOOT).bin of=$(FLOPPY) bs=512 count=1 conv=notrunc

$(BOOT).bin:$(BOOT).asm
	nasm $(BOOT).asm -o $(BOOT).bin
clean:
	-rm $(FLOPPY) $(BOOT).bin $(LOGS)

die:
	@echo "kill bochs"
	@kill -9 `ps aux|grep bochs|grep -v 'grep'|awk '{print $$2}'` 
	@echo "killed"
