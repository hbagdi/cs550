#include <linux/linkage.h>
#include <linux/export.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
//#include <linux/panic.h>

asmlinkage int sys_my_xtime(struct timespec *current_time){

struct timespec kernel_space_mem;
//panic("PLEASE PANIC NOW");
if(access_ok(VERIFY_WRITE, current_time,sizeof(struct timespec)) == 0){
return EFAULT;
}
kernel_space_mem = current_kernel_time();
//getnstimeofday(&kernel_space_mem);
printk(KERN_ALERT "From kernel console\t%ld\n",kernel_space_mem.tv_nsec);
if(copy_to_user(current_time, &kernel_space_mem,sizeof(struct timespec))){
printk(KERN_DEBUG "error in copy to user\n");
return EFAULT;
}
return 0;
}
EXPORT_SYMBOL(sys_my_xtime);
