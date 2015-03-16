#include "tar.h"
#include "stdio.h"
#include "hd.h"
#include "assert.h"
/**
 * @function untar
 * @brief Extract the tar file and store them.
 * @param filename  The tar file
 */
void untar(const char *filename)
{
	printf("[extract '%s'\n", filename);
	int fd = open(filename, O_RDWT);
	assert(fd!=-1);
	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);
	while(1){
		read(fd, buf, SECTOR_SIZE);
		if(buf[0]==0){
			break;
		}
		struct posix_tar_header *phdr = (struct posix_tar_header *)buf;
		//calculate the file size
		char *p = phdr->size;
		int f_len = 0;
		while(*p){
			f_len = (f_len*8)+(*p++-'0');
		}
		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREATE|O_RDWT);
		if(fdout==-1){
			printf("...failed to extract file:%s\n", phdr->name);
			printf(" aborted]\n");
			return;
		}
		printf("...%s (%d bytes\n", phdr->name, f_len);
		while(bytes_left){
			int iobytes = min(chunk, bytes_left);
			read(fd, buf, ((iobytes-1)/SECTOR_SIZE+1) * SECTOR_SIZE);
			write(fdout, buf, iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
	}
	close(fd);
	printf(" done]\n");
}
