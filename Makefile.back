.PHONY:clean img  run die all
BOOT=boot
FLOPPY=a.img
LOGS = *.txt *.log
TMP_DIR = tmp

all:clean $(FLOPPY) run

run:
	bochs -qf bochsrc

img:$(BOOT).bin
	@echo 第一步 生成一个空白软盘镜像
	bximage -fd -size=1.44 -q $(FLOPPY)
	@echo 第二步，使用losetup命令，将镜像作为loop device使用
	losetup /dev/loop0 $(FLOPPY)
	@echo 第三步, 格式化这个loop device
	mkfs.msdos /dev/loop0
	@echo 第四步 检查文件系统
	fsck.msdos /dev/loop0
	@echo 第五步 删除loop device
	losetup -d /dev/loop0
	@echo 第六步 挂载
	-mkdir -p $(TMP_DIR)/nmt/floppy
	mount -o loop,rw $(FLOPPY) $(TMP_DIR)/nmt/floppy
	@echo 第七步，写入文件
	-cp $(BOOT).bin $(TMP_DIR)/nmt/floppy/
	@echo 第八步卸载镜像
	umount $(TMP_DIR)/nmt/floppy
	-rm -rf $(TMP_DIR)	
	
	

$(FLOPPY):img $(BOOT).bin
	#dd if=$(BOOT).bin of=$(FLOPPY) bs=512 count=1 conv=notrunc

$(BOOT).bin:$(BOOT).asm
	nasm $(BOOT).asm -o $(BOOT).bin
clean:
	-rm $(FLOPPY) $(BOOT).bin $(LOGS)

die:
	@echo "kill bochs"
	@kill -9 `ps aux|grep bochs|grep -v 'grep'|awk '{print $$2}'` 
	@echo "killed"
