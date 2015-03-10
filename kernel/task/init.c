#include "type.h"
#include "stdio.h"
#include "assert.h"

void init(){
	int stdin = open("/dev_tty0", O_RDWT);
	assert(stdin == 0);
	int stdout = open("/dev_tty0", O_RDWT);
	assert(stdout == 1);
	int stderr = open("/dev_tty0", O_RDWT);
	assert(stderr == 2);
	char rdbuf[128];
        while(1){
		printf("$");
		int r =	 read(stdin, rdbuf, 70);
		rdbuf[r] = 0;
		if(rdbuf[0]){
			printf(">%s\n", rdbuf);
		}	
 	}
	close(stderr);
	close(stdout);
	close(stdin);   
}
