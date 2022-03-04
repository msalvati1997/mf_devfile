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
#include <linux/spinlock.h>
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
  spinlock_t synchronizer;
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

  the_object = objects + minor;

  if (the_object->op==0) { //non blocking operation 
      spin_trylock(&(the_object->synchronizer));
  } else {
    spin_lock(&(the_object->synchronizer));  //blocking-operation
  }
      if(*off >= OBJECT_MAX_SIZE) {//offset too large
     	spin_unlock(&(the_object->synchronizer));
	    return -ENOSPC;//no space left on device
     } 
      if (the_object->prio == 0) { //high priority stream 
      if(*off > the_object->hi_valid_bytes) {//offset bwyond the current stream size
  	  spin_unlock(&(the_object->synchronizer));
 	    return -ENOSR;//out of stream resources
      } 
   if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off;
      printk("%s: somebody called a high-prio write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
      ret = copy_from_user(&(the_object->hi_prio_stream[*off]),buff,len);
     *off += (len - ret);
      the_object->hi_valid_bytes = *off;
      spin_unlock(&(the_object->synchronizer));
    } else { //low priority stream
        printk("%s: somebody called a low-prio write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
       if(*off > the_object->low_valid_bytes) {//offset bwyond the current stream size
  	   spin_unlock(&(the_object->synchronizer));
 	     return -ENOSR;//out of stream resources
      } 
     if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off;
      ret = copy_from_user(&(the_object->low_prio_stream[*off]),buff,len);
     *off += (len - ret);
      the_object->low_valid_bytes = *off;
     spin_unlock(&(the_object->synchronizer));
    }
  
  return len - ret;
}

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off) {

  int minor = get_minor(filp);
  int ret;
  object_state *the_object;

  the_object = objects + minor;

  if (the_object->op==0) { //non blocking operation 
    spin_trylock(&(the_object->synchronizer));
  }else { 
    spin_lock(&(the_object->synchronizer)); //blocking operation
  }
    if(*off >= OBJECT_MAX_SIZE) {//offset too large
   	spin_unlock(&(the_object->synchronizer));
	  return -ENOSPC;//no space left on device
    } 
    if (the_object->prio == 0) { //high priority stream 
      if(*off > the_object->hi_valid_bytes) {//offset bwyond the current stream size
  	  spin_unlock(&(the_object->synchronizer));
 	    return -ENOSR;//out of stream resources
      } 
   if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off;
     printk("%s: somebody called a high-prio read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

      ret = copy_to_user(buff,&(the_object->hi_prio_stream[*off]),len);
     *off += (len - ret);
      the_object->hi_valid_bytes = *off;
     spin_unlock(&(the_object->synchronizer));
    } else { //low priority stream
         printk("%s: somebody called a low-prio read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

       if(*off > the_object->low_valid_bytes) {//offset bwyond the current stream size
  	   spin_unlock(&(the_object->synchronizer));
 	     return -ENOSR;//out of stream resources
      } 
     if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off;
      ret = copy_to_user(buff,&(the_object->low_prio_stream[*off]),len);
     *off += (len - ret);
      the_object->low_valid_bytes = *off;
     spin_unlock(&(the_object->synchronizer));
    }
  return len - ret;
 
}

static long dev_ioctl(struct file *filp, unsigned int command, unsigned long param) {

  int minor = get_minor(filp);
  object_state *the_object;
  the_object = objects + minor;
 
  if ((int32_t*)param==0) {
    the_object->op=0; //non blocking
  } else {
    the_object->op=1; //blocking
    the_object->TIMEOUT=(int32_t*)param;
  }
  switch (command) {

		case hi_ioctl:
      printk("%s: somebody called an hi-ioctl on dev with [major,minor] number [%d,%d] and command %u \n",MODNAME,get_major(filp),get_minor(filp),command);
      the_object->prio=0;
			break;

		case low_ioctl:
      printk("%s: somebody called an low-ioctl on dev with [major,minor] number [%d,%d] and command %u \n",MODNAME,get_major(filp),get_minor(filp),command);
      the_object->prio=1;
			break;

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
    spin_lock_init(&(objects[i].synchronizer));
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
