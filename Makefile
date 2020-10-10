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
#FDFLAG=-mode=create -fd=1.44M -q
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
	@/bin/echo "make $(FLOPPY)"
	@/bin/echo -e "fd\n1.44\n$(FLOPPY)" | $(BXIMAGE) > /dev/null
#       $(BXIMAGE) $(FDFLAG) $(FLOPPY)
#	$(BXIMAGE) $(HDFLAG) $(DISK)
#	fdisk $(DISK)
#********************************************************#
# disk命令说明：					 #
#	- 用于创建一块硬盘，并开始分区操作		 #
#	- 最简单的分区：				 #
#		1.首先输入n（创建分区）			 #
#		2.输入p创建主分区			 #
#		3.选择分区号，默认1即可			 #
#		4.选择开始磁道默认即可			 #
#		5.选择最后一个磁道，全部		 #
#		6.p打印分区表				 #
#		7.输入t修改分区类型			 #
#		8.选择分区号，这里选择主分区1		 #
#		9.设置启动分区，输入a，选择主分区号1	 #
#		10.写入分区表 输入w			 #
#	-结束						 #
#********************************************************#
disk:
	$(BXIMAGE) $(HDFLAG) $(DISK)
#设置分区表，需要手工输入
	$(FDISK) -C 162 -H 16 -u=cylinders $(DISK)	
clean:
	make clean -C boot
	make clean -C kernel
	make clean -C app
	-rm -rf $(LOGS) $(FLOPPY)
cleanall:clean
	-rm -rf $(DISK)
die:
	@echo "kill bochs"
	@kill -9 `ps aux|grep bochs|grep -v 'grep'|awk '{print $$2}'` 
	@echo "killed"
