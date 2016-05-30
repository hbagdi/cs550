#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "ioctl_definitions.h"

int main(int argc, char const *argv[])
{
	int p = atoi(argv[1]);
	int c;
	if(argc < 2){
		printf("Specify pid to monitor:\nUsage: ./change_pid <pid>");
		exit(0);
	}
	int fd;
	fd = open("/dev/kprobe_mm_fault",O_RDONLY);

	if (fd == -1){
		printf("Error opening device file for ioctl\n");
		exit(0);
	}
	
	if(ioctl(fd, IOCTL_CMD_READ,&c)<0){
		printf("ioctl failed\n");
		perror("\n");
	}
	printf("\nIOCTL READ SUCCESS : Monitroing  :%d \n",c);

	printf("Updating to : %d\n", p);
	if(ioctl(fd, IOCTL_CMD_WRITE,&p)<0){
		printf("ioctl failed\n");
		perror("\n");

	}
	if(ioctl(fd, IOCTL_CMD_READ,&c)<0){
		printf("ioctl failed\n");
		perror("\n");
	}
	printf("\nIOCTL READ SUCCESS : Monitroing  :%d \n",c);
	
	close(fd);
	return 0;
}