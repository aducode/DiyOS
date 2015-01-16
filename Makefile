.PHONY:clean img run die all \
	run-without-loader   \
	run-without-kernel
BOOT=boot
LOADER=loader
KERNEL=kernel
FLOPPY=a.img
LOGS = *.txt

BXIMAGE=bximage -mode=create -fd=1.44M -q

all:clean $(FLOPPY) run

run-without-loader:clean img $(BOOT).bin
	dd if=$(BOOT).bin of=$(FLOPPY) bs=512 count=1 conv=notrunc
	bochs -qf bochsrc
run-without-kernel:clean img $(BOOT).bin $(LOADER).bin
	dd if=$(BOOT).bin of=$(FLOPPY) bs=512 count=1 conv=notrunc
	-mkdir -p tmp/mnt/floppy
	mount -o loop,rw $(FLOPPY) tmp/mnt/floppy
	cp $(LOADER).bin tmp/mnt/floppy
	umount tmp/mnt/floppy/
	rm -rf tmp
	bochs -qf bochsrc
	
run:
	bochs -qf bochsrc

img:
	$(BXIMAGE) $(FLOPPY)

$(FLOPPY):img $(BOOT).bin $(LOADER).bin $(KERNEL).bin
	dd if=$(BOOT).bin of=$(FLOPPY) bs=512 count=1 conv=notrunc
	-mkdir -p tmp/mnt/floppy
	mount -o loop,rw $(FLOPPY) tmp/mnt/floppy/
	cp $(LOADER).bin tmp/mnt/floppy/
	cp $(KERNEL).bin tmp/mnt/floppy/
	umount tmp/mnt/floppy/
	rm -rf tmp

$(BOOT).bin:$(BOOT).asm
	nasm $(BOOT).asm -o $(BOOT).bin
$(LOADER).bin:$(LOADER).asm
	nasm $(LOADER).asm -o $(LOADER).bin
$(KERNEL).bin:$(KERNEL).asm
	nasm $(KERNEL).asm -o $(KERNEL).bin
clean:
	-@rm $(FLOPPY) *.bin $(LOGS)

die:
	@echo "kill bochs"
	@kill -9 `ps aux|grep bochs|grep -v 'grep'|awk '{print $$2}'` 
	@echo "killed"
