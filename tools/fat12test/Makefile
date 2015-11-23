.PHONY:default clean
APP=fat12test
CC=gcc

default:clean $(APP)

clean:
	rm -rf *.o $(APP)

$(APP):fat12test.c fat12.h
	$(CC) -o $(APP) $(APP).c
