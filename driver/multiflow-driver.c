/*
===============================================================================
Driver Name		:		Multiflowdriver
Author			:		MARTINA SALVATI
License			:		GPL
Description		:		LINUX DEVICE DRIVER PROJECT
===============================================================================
*/

/*  
 *  baseline char device driver with limitation on minor numbers - configurable in terms of concurrency 
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
  int mop;
  spinlock_t sl;
  struct mutex block_synchronizer;
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
  printk("%s: somebody called a write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

  if the_object.op==0 { //non blocking operation 

  }

  if the_object.op==1 {  //blocking operation 
     
    mutex_lock(&(the_object->block_synchronizer));
    if(*off >= OBJECT_MAX_SIZE) {//offset too large
   	mutex_unlock(&(the_object->block_synchronizer));
	  return -ENOSPC;//no space left on device
    } 
    if 
    if(*off > the_object->valid_bytes) {//offset bwyond the current stream size
 	  mutex_unlock(&(the_object->block_synchronizer));
	  return -ENOSR;//out of stream resources
   } 
   if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off;
   ret = copy_from_user(&(the_object->stream_content[*off]),buff,len);
   *off += (len - ret);
   the_object->valid_bytes = *off;
   mutex_unlock(&(the_object->block_synchronizer));
  }
  
  return len - ret;

}

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off) {

  int minor = get_minor(filp);
  int ret;
  object_state *the_object;

  the_object = objects + minor;
  printk("%s: somebody called a read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

  //need to lock in any case
  mutex_lock(&(the_object->operation_synchronizer));
  if(*off > the_object->valid_bytes) {
 	 mutex_unlock(&(the_object->operation_synchronizer));
	 return 0;
  } 
  if((the_object->valid_bytes - *off) < len) len = the_object->valid_bytes - *off;
  ret = copy_to_user(buff,&(the_object->stream_content[*off]),len);
  
  *off += (len - ret);
  mutex_unlock(&(the_object->operation_synchronizer));

  return len - ret;
   printk("%s: somebody called a read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

  return 0;

}

static long dev_ioctl(struct file *filp, unsigned int command, unsigned long param) {

  int minor = get_minor(filp);
  object_state *the_object;


  the_object = objects + minor;
  printk("%s: somebody called an ioctl on dev with [major,minor] number [%d,%d] and command %u \n",MODNAME,get_major(filp),get_minor(filp),command);
  switch (command) {

		case hi_op:
      the_object.prio=0;
    
      if (param==0)  //non-blocking operation
      {
        the_object.mop=0
      }
      else  //blocking operation
      {
        the_object.mop=1
      }
      
      
			break;

		case low_op:
      the_object.prio=1;
        if (param==0)  //non-blocking operation
      {
        the_object.mop=0
      }
      else  //blocking operation
      {
        the_object.mop=1
      }
      

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
    mutex_init(&(objects[i].block_synchronizer));
		if(objects[i].hi_prio_stream == NULL || objects[i].low_prio_stream ==NULL ) goto revert_allocation;
	}

	Major = __register_chrdev(0, 0, 256, DEVICE_NAME, &fops);
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
