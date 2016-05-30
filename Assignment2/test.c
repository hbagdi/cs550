#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
//#include <linux/timekeeping.h>
#include <linux/time.h>

int main(){
int y=2;
struct timespec xtime;
struct timeval o_time;
y = syscall(546, &xtime);
gettimeofday(&o_time,NULL);
printf("\nsyscall return  value : %d",y);
printf("\ngetimeofday()\nsec %ld, usec: %ld", o_time.tv_sec,o_time.tv_usec);
printf("\nmy_xtime()\nsec %ld, nsec: %ld\n", xtime.tv_sec, xtime.tv_nsec);
return 0;
}
