.PHONY:default clean
APP=fat12test
CC=gcc
CFLAGS=-Werror

default:clean $(APP)

clean:
	rm -rf *.o $(APP)

$(APP):fat12test.c fat12.h
	$(CC) $(CFLAGS) -o $(APP) $(APP).c
