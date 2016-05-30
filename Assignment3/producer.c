#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define BUFFER_SIZE 100
int main(int argc, char const *argv[])
{
	int fd,i =0;
	char buffer[BUFFER_SIZE];
	fd=open("/dev/pipe-file",O_WRONLY);
	if(fd<0){
		printf("Error opening driver\n");
		return -1;
	}
	while(i++ < 20){
		printf("Enter String:\n");
		scanf("%s",buffer);
		if(write(fd,&buffer,BUFFER_SIZE) > 0){
			printf("Producer writes %s\n",buffer);
		}
		else{
			printf("write failed\n");
		}
	}
	close(fd);
	return 0;
}