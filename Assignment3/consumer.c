#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define BUFFER_SIZE 100
int main(int argc, char const *argv[])
{
	int fd,i =0;
	char buffer[BUFFER_SIZE];
	fd=open("/dev/pipe-file",O_RDONLY);
	if(fd<0){
		printf("Error opening driver\n");
		return -1;
	}
	while(i++ < 5){
		if(read(fd,&buffer,BUFFER_SIZE)> 0 )
			printf("Consumer reads %s\n", buffer);
		else
			printf("error\n");
		
	}
	close(fd);
	return 0;
}