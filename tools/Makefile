#
CMD_TAR=cmd.tar
TARGET=target
IMG=../80m.img
.PHONY:clean all package
all: clean package

clean:
	-@rm $(CMD_TAR)
	-@rm -rf  $(TARGET)

package:$(CMD_TAR)
#	dd if=$(CMD_TAR) of=$(IMG) seek=`echo "obase=10;ibase=16;(

	
$(CMD_TAR):
	make -C ../app
	cp -rf ../app/target .
	tar cvf $(CMD_TAR) -C $(TARGET) `ls $(TARGET)`
	-@rm -rf $(TARGET)
