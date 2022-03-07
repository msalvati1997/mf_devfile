/*
===============================================================================
Driver Name		:		Multiflowdriver
Author			:		MARTINA SALVATI
License			:		GPL
Description		:		LINUX MULTI_FLOW DEVICE DRIVER PROJECT
===============================================================================
*/

#define EXPORT_SYMTAB
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>	
#include <linux/pid.h>		/* For pid types */
#include <linux/tty.h>		/* For the tty declarations */
#include <linux/version.h>	/* For LINUX_VERSION_CODE */
#include <linux/wait.h>
#include <linux/slab.h>
#include "mfdevice_ioctl.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martina Salvati");

#define MODNAME "MULTIFLOW DEV"

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

#define DEVICE_NAME "mf-dev"  /* Device file name in /dev/ - not mandatory  */


static int Major;            /* Major number assigned to broadcast device driver */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session)	MAJOR(session->f_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_inode->i_rdev)
#else
#define get_major(session)	MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_dentry->d_inode->i_rdev)
#endif

typedef struct _object_state{
  int prio;      
  int op;
  int TIMEOUT;
  struct mutex mutex;
	wait_queue_head_t hi_queue; //wait event queue for high pio requests
	wait_queue_head_t low_queue;  //wait event queue for low prio requess
	int hi_valid_bytes;
	int low_valid_bytes;
	char * hi_prio_stream; //the I/O node is a buffer in memory
	char * low_prio_stream; //the I/O node is a buffer in memory

} object_state;

#define MINORS 128
object_state objects[MINORS];

#define OBJECT_MAX_SIZE  (4096) //just one page

/* the actual driver */

static int dev_open(struct inode *inode, struct file *file) {

   int minor;
   minor = get_minor(file);

   if(minor >= MINORS){
	return -ENODEV;
   }

   printk("%s: device file successfully opened for object with minor %d\n",MODNAME,minor);
//device opened by a default nop
   return 0;

}


static int dev_release(struct inode *inode, struct file *file) {

  int minor;
  minor = get_minor(file);


   printk("%s: device file closed\n",MODNAME);
//device closed by default nop
   return 0;

}



static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

  int minor = get_minor(filp);
  int ret;
  object_state *the_object;
  DECLARE_WAITQUEUE(waita, current);
  the_object = objects + minor;

  if (the_object->op==0) { //non blocking operation 
      if(!mutex_trylock(&the_object->mutex)) {
         return -EAGAIN;
      }
  } else { //blocking operation
      if (the_object->prio == 0) { //high priority stream 
        add_wait_queue(&dev->hi_queue, &waita);	     	 
        if (schedule_timeout(the_object->TIMEOUT) == 0) { // if timeout expired - remove from wait queue
                  printk(1, "%s - command timed out.", __func__);
                  ret = -ETIMEDOUT;
                  remove_wait_queue(&the_object->hi_queue, &waita);
                  return ret;
        }
        remove_wait_queue(&the_object->hi_queue, &waita);
        if (ret = mutex_lock_interruptible(&the_object->mutex)) {
           return ret;
        }
        if(*off >= OBJECT_MAX_SIZE) {//offset too large
     	    mutex_unlock(&(the_object->mutex));
	        return -ENOSPC;//no space left on device
         }
         if(*off > the_object->hi_valid_bytes) {//offset bwyond the current stream size
  	       mutex_unlock(&(the_object->mutex));
 	         return -ENOSR;//out of stream resources
          }  
        if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off; {
           printk("current HIGH LEVEL STREAM : %s \n", the_object->hi_prio_stream);
           printk("%s: somebody called a high-prio write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
           *off += the_object->hi_valid_bytes;
           ret = copy_from_user(&(the_object->hi_prio_stream[*off]),buff,len);
           *off += (len - ret);
            the_object->hi_valid_bytes = *off;
            printk("after write HIGH LEVEL STREAM : %s \n", the_object->hi_prio_stream);
            mutex_unlock(&(the_object->mutex)); 
         }
        if(list_empty(&the_object->hi_queue)) {
            wake_up_interruptible(&the_object->low_queue);  //wake up the process attending to the low prio queue
        }
      } else { //low priority stream
        add_wait_queue(&dev->hi_queue, &waita);
        if (schedule_timeout(the_object->TIMEOUT) == 0) { // if timeout expired - remove from wait queue
                  printk(1, "%s - command timed out.", __func__);
                  ret = -ETIMEDOUT;
                  remove_wait_queue(&the_object->low_queue, &waita);
                  return ret;
        }
        if((list_empty(&the_object->hi_queue)) {
              remove_wait_queue(&the_object->low_queue, &waita);
              if(ret = mutex_lock_interruptible(&the_object->mutex)) {
                    return ret;
              }  
        }
        if(*off >= OBJECT_MAX_SIZE) {//offset too large
     	     mutex_unlock(&(the_object->mutex));
	         return -ENOSPC;//no space left on device
        }
       if(*off > the_object->low_valid_bytes) {//offset bwyond the current stream size
  	     mutex_unlock(&(the_object->mutex));
 	       return -ENOSR;//out of stream resources
       } 
       if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off; {
        printk("current LOW LEVEL STREAM : %s\n", the_object->low_prio_stream);
        printk("%s: somebody called a low-prio write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
        *off += the_object->low_valid_bytes;
        ret = copy_from_user(&(the_object->low_prio_stream[*off]),buff,len);
        *off += (len - ret);
        the_object->low_valid_bytes = *off;
         printk("after write LOW LEVEL STREAM : %s\n", the_object->low_prio_stream);
        mutex_unlock(&(the_object->mutex));
      } 
    }
  }
  return len - ret;
}

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off) {

  int minor = get_minor(filp);
  int ret;
  object_state *the_object;

  the_object = objects + minor;

   if (the_object->op==0) { //non blocking operation 
      printk("non blocking op\n");
      if(mutex_trylock(&the_object->mutex)==false) 
          return -EBUSY;  // return to the caller   
  } else { //blocking operation
      if (the_object->prio == 0) { //high priority stream  
        printk("process %d(%s) is going to sleep in high prio wait queue- TIMEOUT: %d \n",current->pid, current->comm, the_object->TIMEOUT); 
        wait_event_timeout(the_object->hi_queue, 0, the_object->TIMEOUT);
        mutex_lock(&the_object->mutex); 
        if(*off > the_object->hi_valid_bytes) {
            	mutex_unlock(&(the_object->mutex));
	            return 0;
        } 
        if((the_object->hi_valid_bytes - *off) < len) len = the_object->hi_valid_bytes - *off; {
         printk("current HIGH LEVEL STREAM : %s \n", the_object->hi_prio_stream);
         printk("%s: somebody called a high-prio read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
         ret = copy_to_user(buff,&(the_object->hi_prio_stream[*off]),len);
         off += (len - ret);
         the_object->hi_prio_stream+=len;
         the_object->hi_valid_bytes-=len;
         printk("after read HIGH LEVEL STREAM : %s \n", the_object->hi_prio_stream);
         mutex_unlock(&(the_object->mutex)); 
         }
      }
      else { //low priority stream
        printk("process %d(%s) is going to sleep to low prio queue- TIMEOUT: %d \n",current->pid, current->comm, the_object->TIMEOUT); 
        wait_event_timeout(the_object->low_queue, 0, the_object->TIMEOUT);
        mutex_lock(&the_object->mutex);
        if(*off > the_object->low_valid_bytes) {
            	 mutex_unlock(&(the_object->mutex));
	             return 0;
        } 
        if((the_object->low_valid_bytes - *off) < len) len = the_object->low_valid_bytes - *off; {
         printk("current LOW LEVEL STREAM : %s \n", the_object->low_prio_stream);
         printk("%s: somebody called a low-prio read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
         ret = copy_to_user(buff,&(the_object->low_prio_stream[*off]),len);
         off += (len - ret);
         the_object->low_prio_stream+=len;
         the_object->low_valid_bytes-=len;
        printk("after read LOW LEVEL STREAM : %s \n", the_object->low_prio_stream);
         mutex_unlock(&(the_object->mutex)); 
       }
      }
  }
  return 0;
}

static long dev_ioctl(struct file *filp, unsigned int command, unsigned long arg) {

  int minor = get_minor(filp);
  object_state *the_object;
  the_object = objects + minor;
 
  switch (command) {

    case IOCTL_RESET :
       the_object->prio=DEFAULT_PRIO;
       the_object->op=DEFAULT_OP;
       the_object->TIMEOUT=DEFAULT_TIMER;
       break;

		case IOCTL_HIGH_PRIO:
    	the_object->prio = 0;
			break;
		case IOCTL_LOW_PRIO:
    	the_object->prio = 1;
			break;
    case IOCTL_BLOCKING :
      the_object->op = 1;
			break;
    case IOCTL_NO_BLOCKING:
     	the_object->op = 0;
			break;
    case IOCTL_SETTIMER :
      int timer =0;
      timer = the_object->TIMEOUT;
    	the_object->TIMEOUT= arg;
      printk("Set timeout : %d", the_object->TIMEOUT);
		default:
			return -ENOTTY;
	}

  //do here whathever you would like to control the state of the device
  return 0;

}



static struct file_operations fops = {
  .owner = THIS_MODULE,//do not forget this
  .write = dev_write,
  .read = dev_read,
  .open =  dev_open,
  .release = dev_release,
  .unlocked_ioctl = dev_ioctl
};



int init_module(void) {

int i;

	//initialize the drive internal state
	for(i=0;i<MINORS;i++){
		objects[i].hi_valid_bytes = 0;
		objects[i].low_valid_bytes = 0;
		objects[i].hi_prio_stream = NULL;
		objects[i].low_prio_stream = NULL;
		objects[i].hi_prio_stream = (char*)__get_free_page(GFP_KERNEL);
		objects[i].low_prio_stream = (char*)__get_free_page(GFP_KERNEL);
   	mutex_init(&(objects[i].mutex));
    init_waitqueue_head(&(objects[i].hi_queue));
    init_waitqueue_head(&(objects[i].low_queue));
		if(objects[i].hi_prio_stream == NULL || objects[i].low_prio_stream ==NULL ) goto revert_allocation;
	}

	Major = __register_chrdev(0, 0, 128, DEVICE_NAME, &fops);
	//actually allowed minors are directly controlled within this driver

	if (Major < 0) {
	  printk("%s: registering device failed\n",MODNAME);
	  return Major;
	}

	printk(KERN_INFO "%s: new device registered, it is assigned major number %d\n",MODNAME, Major);

	return 0;

  revert_allocation:
	for(;i>=0;i--){
		free_page((unsigned long)objects[i].hi_prio_stream);
		free_page((unsigned long)objects[i].low_prio_stream);
	}
	return -ENOMEM;




}

void cleanup_module(void) {

	int i;
	for(i=0;i<MINORS;i++){
		free_page((unsigned long)objects[i].low_prio_stream);
		free_page((unsigned long)objects[i].hi_prio_stream);
	}

	unregister_chrdev(Major, DEVICE_NAME);

	printk(KERN_INFO "%s: new device unregistered, it was assigned major number %d\n",MODNAME, Major);

	return;

}
