.PHONY:clean img run die all
BOOT=boot
LOADER=loader
FLOPPY=a.img
LOGS = *.txt

BXIMAGE=bximage -mode=create -fd=1.44M -q

all:clean $(FLOPPY) run

run:
	bochs -qf bochsrc

img:
	$(BXIMAGE) $(FLOPPY)

$(FLOPPY):img $(BOOT).bin $(LOADER).bin
	dd if=$(BOOT).bin of=$(FLOPPY) bs=512 count=1 conv=notrunc
	-mkdir -p tmp/mnt/floppy
	mount -o loop,rw $(FLOPPY) tmp/mnt/floppy/
	cp $(LOADER).bin tmp/mnt/floppy/
	umount tmp/mnt/floppy/
	rm -rf tmp

$(BOOT).bin:$(BOOT).asm
	nasm $(BOOT).asm -o $(BOOT).bin
$(LOADER).bin:$(LOADER).asm
	nasm $(LOADER).asm -o $(LOADER).bin

clean:
	-@rm $(FLOPPY) *.bin $(LOGS)

die:
	@echo "kill bochs"
	@kill -9 `ps aux|grep bochs|grep -v 'grep'|awk '{print $$2}'` 
	@echo "killed"
