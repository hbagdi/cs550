/*
 * Copyright (C) 1998 Alessandro Rubini (rubini@linux.it)
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#define MAX_Q_SIZE 5


#define DRIVER_AUTHOR "hbagdi1 and rkugver1@binghamton.edu"
#define DRIVER_DESC   "A sample driver"
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

static struct miscdevice pipe_dev;
static DECLARE_WAIT_QUEUE_HEAD(pro_wq);
static DECLARE_WAIT_QUEUE_HEAD(con_wq);
static DEFINE_MUTEX(mtx);

int size = MAX_Q_SIZE; 
struct queue { 
    int size;      //Maximum slots in buffer. Can be passed from cmd line during insmod
    int n;  //number of currently occupied slots
    int r;      //read ptr
    int w;      //write ptr
    char **arr;  //array of pointers to strings
};

struct queue q;

int init_queue( void )
{
   q.size = size;
   q.arr =  kmalloc(sizeof( char* )* q.size, GFP_KERNEL);
   if(q.arr== NULL){
       return -1;
   }
   memset(q.arr, 0, sizeof( char* )* q.size);
   q.r = 0;   //read index
   q.w = 0;   //write index
   q.n = 0;
   return 0;
}


module_param(size, int, 0);
MODULE_PARM_DESC(q_size, "number of slots in pipe");

int reset_queue(void)
{
    int i = 0;
    printk(KERN_ALERT "pipe_drv: %s %d : Reseting Queue\n",__FUNCTION__,__LINE__);
    for( i = 0; i < q.size; i++ )   {
        if( q.arr[i] != NULL )
            kfree(q.arr[i]);
    }
    kfree(q.arr);
    init_queue();
    return 0;
}
int pipe_open (struct inode *inode, struct file *file)
{
    //MOD_INC_USE_COUNT;
    printk(KERN_ALERT "pipe_drv: open called %s %d \n",__FUNCTION__,__LINE__);
    return 0;
}

void pipe_close (struct inode *inode, struct file *filp)
{
    //MOD_DEC_USE_COUNT;
    printk(KERN_ALERT "pipe_drv: close called %s %d \n",__FUNCTION__,__LINE__);
}


ssize_t pipe_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    int len = 0;
    //get the lock
    if ( mutex_lock_interruptible(&mtx)){
        return -EINTR;
    }
    printk(KERN_DEBUG "process%i (%s): checking if data available. count = %zu\n", \
            current->pid, current->comm, count);
    while( q.n ==  0 ) {
        mutex_unlock(&mtx);     //giveup lock before sleeping
        if(wait_event_interruptible( con_wq, q.n > 0 )){        
            printk(KERN_DEBUG "process %i (%s) got signal\n", current->pid, current->comm);
            return -ERESTARTSYS;    //for Ctrl+C
        }
        if ( mutex_lock_interruptible(&mtx)){   //workup. get lock before checking why.
            return -EINTR;
        }

    }
    len =  strlen(q.arr[q.r]) + 1;  //we have the lock and data available to write
    printk(KERN_DEBUG "string len = %d\n", len-1);
    if( len < count ) // if user tries to read more than the string handle it.
        count = len;
    if (copy_to_user( buf, q.arr[q.r],count)) {
       //up (&dev->sem);
       //!!!!!!!! release lock here
       return -EFAULT;
    }
    printk(KERN_DEBUG "process%i (%s): copied %s from slot %d\n", \
           current->pid, current->comm, buf, q.r);
    kfree(q.arr[q.r]);
    q.r++;   //more read head
    if( q.r == q.size)    q.r = 0; //wrap around
    q.n--;   //decremnt total count 
    mutex_unlock(&mtx);  // releas the lock and then wakeup others.
                        // Otherwise woken up processes cant get theis lock
    printk(KERN_DEBUG "process %i (%s) awakening the writers...\n",
            current->pid, current->comm);
    wake_up_interruptible(&pro_wq);
    return count; /* succeed, to avoid retrial */
}

ssize_t pipe_write (struct file *filp, const char __user *buf, size_t count,
                        loff_t *f_pos)
{
    //FOR DEBUG ONLY
    // int i = 0;
    // for( i = 0; i < count; i++) {
    //     printk(KERN_DEBUG "buf[%d] = %d\n", i, buf[i]);
    // }
    if( buf[0] == '0') {     // if user writes the magic number 0 then reset.
        reset_queue();
        return count;
    }
    //sorme error checking
    if( count <= 0 )
        return 0;
    //get the lock
    if ( mutex_lock_interruptible(&mtx)){
        return -EINTR;
    }
    printk(KERN_DEBUG "process%i (%s): checking if slots available. count = %zu\n", \
            current->pid, current->comm, count);
    while( q.n ==  q.size ) {
        mutex_unlock(&mtx);     //giveup lock before sleeping
        if(wait_event_interruptible(pro_wq, q.n < q.size)){        
            printk(KERN_DEBUG "process %i (%s) got signal\n", current->pid, current->comm);
            return -ERESTARTSYS;    //for Ctrl+C
        }
        if ( mutex_lock_interruptible(&mtx)){   //workup. get lock before checking why.
            return -EINTR;
        }

    }
   //we have the lock and slot available to write
   q.arr[q.w] =  kmalloc(sizeof( char) * count, GFP_KERNEL);
   if(q.arr[q.w] == NULL){
       //!!!!!!!! release lock here
       return -1;
   }
   if (copy_from_user(q.arr[q.w], buf, count)) {
       //up (&dev->sem);
       //!!!!!!!! release lock here
       return -EFAULT;
   }    
   //if the input was not null terminated, terminate it. This cant done to buf directly 
   q.arr[q.w][count -1] = '\0';

   printk(KERN_DEBUG "process%i (%s): copied %s to slot %d\n", \
           current->pid, current->comm,q.arr[q.w], q.w);
   q.w++;   //more write head
   if( q.w == q.size)    q.w = 0; //wrap around
   q.n++; 
   mutex_unlock(&mtx);  // releas the lock and then wakeup others.
                        // Otherwise woken up processes cant get theis lock
   printk(KERN_DEBUG "process %i (%s) awakening the readers...\n",
            current->pid, current->comm);
   wake_up_interruptible(&con_wq);
   return count; /* succeed, to avoid retrial */

}



struct file_operations pipe_fops = {
	.owner =	THIS_MODULE,
	.read =		pipe_read,
 	.write =	pipe_write,
	.open =		pipe_open,
};

int init_module(void)
{
    int retval;
    pipe_dev.minor = MISC_DYNAMIC_MINOR;
    pipe_dev.name = "pipe-file";    //This will be the name od the /dev/ file. 
                                    //We may need to take it as input arguement for insmod
    pipe_dev.fops = &pipe_fops;
    retval = misc_register(&pipe_dev);
    printk(KERN_ALERT "pipe_drv: init called. size = %d.  %s %d \n", size, __FUNCTION__,__LINE__);
    if (retval) {
        printk(KERN_ALERT "pipe_drv: register error %s %d \n",__FUNCTION__,__LINE__);
        return retval;
    }
    if( init_queue( ) < 0)  {
        printk(KERN_ALERT "pipe_drv: init_queue error %s %d \n",__FUNCTION__,__LINE__);
        return -1;
    }
    mutex_init(&mtx);
    init_waitqueue_head(&pro_wq);
    init_waitqueue_head(&con_wq);
    return 0;
}

void cleanup_module(void)
{
    misc_deregister(&pipe_dev);
    printk(KERN_ALERT "pipe_drv: cleanup  called %s %d \n",__FUNCTION__,__LINE__);
}

