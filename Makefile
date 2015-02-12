.PHONY:clean cleanall boot kernel img disk run die \
	run-without-loader   \
	run-without-kernel
BOOT=boot/boot.bin
LOADER=boot/loader.bin
KERNEL=kernel/kernel.bin
FLOPPY=a.img
DISK=80m.img
LOGS = *.txt *.log

BXIMAGE=bximage
FDISK=fdisk
FDFLAG=-mode=create -fd=1.44M -q
HDFLAG=-mode=create -hd=80M -q


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
	$(BXIMAGE) $(FDFLAG) $(FLOPPY)
#	$(BXIMAGE) $(HDFLAG) $(DISK)
#	fdisk $(DISK)
disk:
	$(BXIMAGE) $(HDFLAG) $(DISK)
#分区
	$(FDISK) -C 162 -H 16 -u=cylinders $(DISK)	
clean:
	make clean -C boot
	make clean -C kernel
	-rm -rf $(LOGS) $(FLOPPY)
cleanall:clean
	-rm -rf $(DISK)
die:
	@echo "kill bochs"
	@kill -9 `ps aux|grep bochs|grep -v 'grep'|awk '{print $$2}'` 
	@echo "killed"
