.PHONY:clean boot kernel img run die \
	run-without-loader   \
	run-without-kernel
BOOT=boot/boot.bin
LOADER=boot/loader.bin
KERNEL=kernel/kernel.bin
FLOPPY=a.img
LOGS = *.txt *.log

BXIMAGE=bximage -mode=create -fd=1.44M -q


run:clean img boot kernel
	dd if=$(BOOT) of=$(FLOPPY) bs=512 count=1 conv=notrunc
	-mkdir -p tmp/mnt/floppy
	mount -o loop,rw $(FLOPPY) tmp/mnt/floppy/
	cp $(LOADER) tmp/mnt/floppy/
	cp $(KERNEL) tmp/mnt/floppy/
	umount tmp/mnt/floppy/
	rm -rf tmp
	bochs -qf config/bochsrc

run-without-loader:clean img boot
	dd if=$(BOOT) of=$(FLOPPY) bs=512 count=1 conv=notrunc
	bochs -qf config/bochsrc
run-without-kernel:clean img boot
	dd if=$(BOOT) of=$(FLOPPY) bs=512 count=1 conv=notrunc
	-mkdir -p tmp/mnt/floppy
	mount -o loop,rw $(FLOPPY) tmp/mnt/floppy
	cp $(LOADER) tmp/mnt/floppy
	umount tmp/mnt/floppy/
	rm -rf tmp
	bochs -qf config/bochsrc
boot:
	make -C boot/
kernel:
	make -C kernel/

img:
	$(BXIMAGE) $(FLOPPY)

clean:
	make clean -C boot
	make clean -C kernel
	-rm -rf $(LOGS) $(FLOPPY)

die:
	@echo "kill bochs"
	@kill -9 `ps aux|grep bochs|grep -v 'grep'|awk '{print $$2}'` 
	@echo "killed"
