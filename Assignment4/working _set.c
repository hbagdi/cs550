/*
 *This program implements a benchmark that dirty #pages to vary writable working set.
 *
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#define DEBUG

int main(int argc, char ** argv){
	
	if(argv[1] == NULL){
	printf("\nGive size in MBs\n");
	return 1;	
	}
	int total_mb = atoi(argv[1]);
	printf("MB Given : %d\n", total_mb);
	// size of a page is supposed as 4kb constant		
	//calculate total pages
	unsigned long total_pages   = total_mb*256;
	
	 // quantum: size to be skipped in terms of int*
	int quantum = 	4*1024 / sizeof(int);
	
	//recording time	
	struct timeval start, end;
	//allocate memory 
	int * array;
	if(!(array = malloc(total_pages*4*1024))) {
	        perror("malloc");
	 return 1;
	}
	
	//random number which will be written to mem
	//random pointer is random pointer inside of a page. so we dirty any random 4 bytes inside a page	
	int rand_no;
	int rand_pointer;

	//current page being dirtied inside the array	
	int page_no =0;
    	
	time_t t;
	srand(time(NULL));
#ifdef DEBUG
	printf("\nSettings \nTotal MB: %d \nTotal Pages: %lu\nQuantum: %d\n", total_mb, total_pages, quantum);
#endif 
	gettimeofday(&start,NULL);
	while(1)
	{
		page_no = 0;
		
		while(page_no < total_pages)
		{
			rand_no = rand() % 10000;
			rand_pointer = rand() % 1024;
			 array[page_no*quantum + rand_pointer] = rand_no;
			 array[page_no*quantum + (rand() % 1024)] = rand_no;
#ifdef DEBUG
			 printf("[%d]Writing in page number : %d at index: %d, random number %d \n",page_no*quantum + rand_pointer, page_no, rand_pointer, array[page_no*quantum + rand_pointer]);
#endif 			
			page_no++;
		}	
		
		gettimeofday(&end,NULL);
		double t1 = start.tv_sec + (start.tv_usec / 1000000.0);
		double t2 = end.tv_sec + (end.tv_usec / 1000000.0);

		double elapsed_time = (t2-t1);

		//if(elapsed_time >= (600))//wait for 600 seconds
		//{
		//	break; // break the while loop
		//}	
	}	
	
	free(array);
return 0;
}
