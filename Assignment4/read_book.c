#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "ioctl_definitions.h"
#define MAX_BOOK_SIZE 10000
#define COPY_QUANTUM 100

struct tuple{
unsigned long address;
struct timeval time_stamp;
};
struct tuple book[COPY_QUANTUM];
int head = 0;

int roundcount=0;
struct param{
  struct tuple* b;
  int count;
};



int main(int argc, char const *argv[])
{
	int i,j=0;
	int fd;
	int ret;
	//printf("\nSize of tuple : %d",(int)sizeof(struct tuple));
	struct param entire_book = {
		.b = book,
		.count =1,
	};
	fd = open("/dev/kprobe_mm_fault",O_RDONLY);

	if (fd == -1){
		printf("Error opening device file for ioctl\n");
		exit(0);
	}
	
	while(1){
	//	printf("Round:%d\n",roundcount++);
		sleep(5);
		ret  = ioctl(fd, IOCTL_CMD_BOOK_READ,&entire_book);
		if(ret<0){
			printf("ioctl failed\n");
			perror("\n");

		}
		else if(ret == 1 ){
			printf("copying finished\n");
			exit(0);
		}
		else{
		//	printf("current_head : %d\n",entire_book.count);
			for(i=0;i<COPY_QUANTUM;i++){
				printf("\n%d- Address: %lu Time: %ld.%06ld ",j++,book[i].address,(long int)(book[i].time_stamp.tv_sec),(long int)(book[i].time_stamp.tv_usec));
			}
		}
	}

	return 0;
}
