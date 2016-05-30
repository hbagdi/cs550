#include <linux/module.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include "ioctl_definitions.h"
#define MAX_BOOK_SIZE 10000
#define COPY_QUANTUM 100
#define DRIVER_AUTHOR "hbagdi@binghamton.edu"
#define DRIVER_DESC   "Probing kprobe_mm_fault using kprobe"
static DEFINE_MUTEX(mtx_pid);
static DEFINE_MUTEX(mtx_book);
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
static struct miscdevice kprobe_mm_fault;
int counter = 0;
int q_dmsg =0;
int current_copy_head =0;
int copy_round = 0;
//DS to hold VA and time
struct tuple{
unsigned long address;
struct timeval time_stamp;
};
struct tuple book[MAX_BOOK_SIZE];
int head = 0;
int more_space = 1;
int pid_to_monitor = -123;


struct param{
  struct tuple* b;
  int count;
};

void update_book(unsigned long virtual_address)
{
  struct timeval current_time;
  mutex_lock_interruptible(&mtx_book);
  if(more_space)
  { 
  book[head].address = virtual_address;
  do_gettimeofday(&current_time);
  book[head].time_stamp = current_time;
  head++;
  if(head == MAX_BOOK_SIZE)
    more_space =0;
  }
  mutex_unlock(&mtx_book);
  return;
}

int update_pid(int* p )
{
  int pid_to_update = -2;
  printk(KERN_ALERT "Printing p :%d",*p);
  if(copy_from_user(&pid_to_update,p,sizeof(int)))
  {
    printk(KERN_ALERT "\nkprobe_mm_fault: copy_to_user failed");
    return -EFAULT;
  }
  printk(KERN_ALERT "\nkprobe_mm_fault: new value of PID %d\n",pid_to_update);

  mutex_lock_interruptible(&mtx_pid);
  pid_to_monitor = pid_to_update;
  mutex_unlock(&mtx_pid);

  mutex_lock_interruptible(&mtx_book);
  head = 0;
  current_copy_head =0;
  mutex_unlock(&mtx_book);
  printk(KERN_ALERT "\nkprobe_mm_fault: ioctl write success %d",pid_to_update);
  printk(KERN_ALERT "\nkprobe_mm_fault: Now monitoring : %d",pid_to_monitor);
  return 0;
}

int read_pid(int* p)
{
  int pid_to_copy = -2;
  mutex_lock_interruptible(&mtx_pid);
  pid_to_copy = pid_to_monitor;
  mutex_unlock(&mtx_pid);
  if(copy_to_user(p,&pid_to_copy,sizeof(int)))
  {
    printk(KERN_ALERT "\nkprobe_mm_fault: copy_to_user failed");
    return -EFAULT;
  }
  printk(KERN_ALERT "kprobe_mm_fault: ioctl read success \n");
  return 0;
	
}

int read_book(struct param* param1)
{
  mutex_lock_interruptible(&mtx_book);
  if(current_copy_head >= head)
  {
    printk(KERN_ALERT "\nkprobe_mm_fault: copying book finished (rounds:%d)",copy_round);
    return 1; // 1 signfies that copy finished
  }
  if(copy_to_user((param1->b),&(book[current_copy_head]),COPY_QUANTUM*sizeof(struct tuple)))
  {
    printk(KERN_ALERT "\nkprobe_mm_fault: copy_to_user(book) failed");
    mutex_unlock(&mtx_book); 
    return -EFAULT;
  }

  if(copy_to_user(&(param1->count),&head,sizeof(int)))
  {
    printk(KERN_ALERT "\nkprobe_mm_fault: copy_to_user(head) failed");
    mutex_unlock(&mtx_book);    	
    return -EFAULT;
  }
  mutex_unlock(&mtx_book);
  printk(KERN_ALERT "kprobe_mm_fault: success,round(%d) current_copy_head(%d),\n",++copy_round,current_copy_head);
  current_copy_head += COPY_QUANTUM;
  return 0;

}
static long device_ioctl( struct file *file,
       unsigned int cmd, unsigned long ioctl_param)
{
  switch(cmd)
  {
      case IOCTL_CMD_BOOK_READ:   printk(KERN_ALERT "kprobe_mm_fault: ioctl book head called");
                            if(read_book((struct param*) ioctl_param)){
                              return -1;
                            }
                            break;
    case IOCTL_CMD_WRITE:   printk(KERN_ALERT "kprobe_mm_fault: ioctl write pid called");
                            if(update_pid((int*) ioctl_param)){
                              return -1;
                            }
                            break;
     case IOCTL_CMD_READ:   printk(KERN_ALERT "kprobe_mm_fault: ioctl read pid called");
                            if(read_pid((int*) ioctl_param)){
                              return -1;
                            }
                            break;       
    default: 
                  printk(KERN_ALERT "kprobe_mm_fault: ioctl - something went wrong");                 
  }
  return 0;
}


static int j_handle_mm_fault(struct mm_struct *mm,
  struct vm_area_struct *vma,unsigned long address, unsigned int flags)
{
  //printk(KERN_ALERT "\nFault address: %lu, current process: %d counter : %d",address,current->pid, ++counter);
  //printk(KERN_ALERT "\npid:  %d\n", current->pid);  
 if(current->pid == pid_to_monitor)
  {
    counter++;
    //if((counter %100) == 0)
    //  printk(KERN_ALERT "\ncounter : %d\n", counter);
    printk(KERN_ALERT "\nFault address: %lu, from process: %d",address,current->pid);
    update_book(address);
  }
  jprobe_return();
  return 0;
}


static struct jprobe jprobe_handle_mm_fault = {
  .entry = j_handle_mm_fault,
  .kp = {
      .symbol_name = "handle_mm_fault"
  },
};

//ioctl functionalities to read and change pids
int kprobe_open (struct inode *inode, struct file *file)
{
    printk(KERN_ALERT "kprobe_mm_fault: open called %s %d \n",__FUNCTION__,__LINE__);
    return 0;
}

void kprobe_close (struct inode *inode, struct file *filp)
{
    printk(KERN_ALERT "kprobe_mm_fault: close called %s %d \n",__FUNCTION__,__LINE__);
}

struct file_operations pid_input = {
	.owner =	THIS_MODULE,
	.open =		kprobe_open,
  .unlocked_ioctl = device_ioctl
};

int init_module(void)
{
    int ret;
    kprobe_mm_fault.minor = MISC_DYNAMIC_MINOR;
    kprobe_mm_fault.name = "kprobe_mm_fault";   
    kprobe_mm_fault.fops = &pid_input;
    ret = misc_register(&kprobe_mm_fault);
    if (ret)
    {
        printk(KERN_ALERT "kprobe_mm_fault: register error %s %d \n",__FUNCTION__,__LINE__);
        return ret;
    }
    if( register_jprobe(&jprobe_handle_mm_fault)< 0)
    {
        printk(KERN_ALERT "kprobe_mm_fault: jprobe registration error %s %d \n",__FUNCTION__,__LINE__);
        return -1;
    }
    mutex_init(&mtx_pid);
    printk(KERN_ALERT "kprobe_mm_fault: init success.  \n");
    return 0;
}

void cleanup_module(void)
{
    unregister_jprobe(&jprobe_handle_mm_fault);
    misc_deregister(&kprobe_mm_fault);
    printk(KERN_ALERT "kprobe_mm_fault: cleanup  called %s %d \n",__FUNCTION__,__LINE__);
}