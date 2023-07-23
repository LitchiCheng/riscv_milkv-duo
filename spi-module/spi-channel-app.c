#include "stdio.h"
#include "stdint.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	int fd;
	int ret = 0;
	char* filename = "/dev/spi_channel";
	fd = open(filename, O_RDWR);
	if(fd < 0) {
		printf("can't open file %s\r\n", filename);
		return -1;
	}

	while (1) {
		uint8_t buffer[7] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07};
		printf("send buffer\r\n");
		for(int i=0;i<8;i++){
			printf("%x ",buffer[i]);
		}
		printf("\r\n");
		ret = write(fd, buffer, sizeof(buffer));
		if(ret > 0) {
			printf("read buffer\r\n");
			for(int i=0;i<8;i++){
				printf("%x ",buffer[i]);
			}
			printf("\r\n");
		}
		usleep(100000); 
	}
	close(fd);
	return 0;
}
