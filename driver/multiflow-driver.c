/*
===============================================================================
Driver Name		:		multiflow-driver
Author			:		MARTINA SALVATI
License			:		GPL
Description		:		LINUX MULTI_FLOW DEVICE DRIVER PROJECT
===============================================================================
*/

#define EXPORT_SYMTAB
#include "multiflow-driver.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martina Salvati");

#define MULTIFLOWDRIVER_WORKQUEUE "multiflowdriver_workqueue"

//driver operations
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static long dev_ioctl(struct file *filp, unsigned int command, unsigned long arg);

static void deferred_work(struct work_struct *work);

static int enabled;
static int nthreads;
static int nbytes;
static int Major;            /* Major number assigned to broadcast device driver */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session)	MAJOR(session->f_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_inode->i_rdev)
#else
#define get_major(session)	MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_dentry->d_inode->i_rdev)
#endif


struct __deferred_work_item {
        struct file *filp;
        char * bff;
        size_t len; 
        long long int off;   
        struct work_struct	w;
};
typedef struct __deferred_work_item deferred_work_t;

typedef struct _object_state{
  int prio;      
  int op;
  int TIMEOUT;
  unsigned long jiffies;
  struct mutex mutex_hi;
  struct mutex mutex_low;
	wait_queue_head_t hi_queue; //wait event queue for high pio requests
	wait_queue_head_t low_queue;  //wait event queue for low prio requess
	int hi_valid_bytes;
  struct workqueue_struct *wq;
	int low_valid_bytes;
	char * hi_prio_stream; //the I/O node is a buffer in memory
	char * low_prio_stream; //the I/O node is a buffer in memory
} object_state;



#define MINORS 128
object_state objects[MINORS];

#define OBJECT_MAX_SIZE  (4096) //just one page

static void deferred_work(struct work_struct *work) {
   int minor;
   object_state *the_object;
   PINFO("[Deferred work]=> PID: %d; NAME: %s\n", current->pid, current->comm);
   deferred_work_t *tw;
   tw = container_of(work, deferred_work_t, w);
   if (!tw) {
		pr_err("%s: Null pointer!!\n", __func__);
		return;
	  }
   minor = get_minor(tw->filp);
   the_object = objects + minor;
  if (the_object->op==0) { 
        if(mutex_trylock(&the_object->mutex_low)) { //non blocking 
           PERR("non blocking operation : can't do the operation\n");;
        } 
     }   else {
      if (mutex_lock_interruptible(&the_object->mutex_low)) { //blocking operation //sync
           PERR("mutex problem\n");
      }
     }
   if((tw->off) >= OBJECT_MAX_SIZE) {//offset too large
          mutex_unlock(&the_object->mutex_low);
	        PERR("offset too large\n");
        }
   if((tw->off) > the_object->low_valid_bytes) {//offset bwyond the current stream size
         mutex_unlock(&the_object->mutex_low);
 	       PERR("out of stream resources\n");
       }  
   if((OBJECT_MAX_SIZE - (tw->off)) < (tw->len)) (tw->len) = OBJECT_MAX_SIZE - (tw->off); {
        PDEBUG("current LOW LEVEL STREAM : %s\n", the_object->low_prio_stream);
        PDEBUG("offset %lld, len %d , buff %s\n",(tw->off), tw->len, tw->bff);
        PINFO("somebody called a low-prio deferred - write on dev with [major,minor] number [%d,%d]\n",get_major(tw->filp),get_minor(tw->filp));
        (tw->off)  += the_object->low_valid_bytes;
        strncat(&(the_object->low_prio_stream[(tw->off)]),(tw->bff) ,(tw->len));
        tw->off +=tw->len;
        the_object->low_valid_bytes = (tw->off); 
        PDEBUG("after deferred-write LOW LEVEL STREAM : %s\n", the_object->low_prio_stream);
        kfree(tw);
        mutex_unlock(&(the_object->mutex_low));
      } 
 }
 

/* the actual driver */

static int dev_open(struct inode *inode, struct file *file) {

   int minor;
   minor = get_minor(file);

   if(minor >= MINORS){
    	return -ENODEV;
   }

   PINFO("device file successfully opened for object with minor %d\n",minor);
   //device opened by a default nop
   return 0;
}


static int dev_release(struct inode *inode, struct file *file) {

   int minor;
   minor = get_minor(file);


   PINFO("device file closed\n");
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
   if (the_object->prio == 0) {
      if(mutex_trylock(&the_object->mutex_hi)) {
         return -EAGAIN;
      } else {
         goto write_hi;
      }
   } else {
      if(mutex_trylock(&the_object->mutex_hi)) {
         return -EAGAIN;
      } else {
         goto write_low;
     } }
     } else { //blocking operation
      if (the_object->prio == 0) { //high priority stream 
        add_wait_queue(&the_object->hi_queue, &waita);	     	 
        if (schedule_timeout(the_object->jiffies) == 0) { // if timeout expired - remove from wait queue
               PINFO("%s - command timed out.", __func__);
               remove_wait_queue(&the_object->hi_queue, &waita);
               return ETIMEDOUT;
        }
        if (mutex_lock_interruptible(&the_object->mutex_hi)) { //sync operation
           return -ERESTARTSYS;
        } else {
            remove_wait_queue(&the_object->hi_queue, &waita);
            goto write_hi;
        }
      } else { //low priority stream
        if (schedule_timeout(the_object->jiffies) == 0) {  //if timeout expired - remove from wait queue
                  PINFO("%s - command timed out.", __func__);
                  ret = -ETIMEDOUT;
                  return ret;
        } else {
           goto write_low;
        }
    }
  }
  return len - ret;

  write_hi : 
        if(*off >= OBJECT_MAX_SIZE) {//offset too large
     	    mutex_unlock(&(the_object->mutex_hi));
	        return -ENOSPC;//no space left on device
         }
         if(*off > the_object->hi_valid_bytes) {//offset bwyond the current stream size
  	       mutex_unlock(&(the_object->mutex_hi));
 	         return -ENOSR;//out of stream resources
          }  
        if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off; {
           PDEBUG("current HIGH LEVEL STREAM : %s \n", the_object->hi_prio_stream);
           PINFO("somebody called a high-prio write on dev with [major,minor] number [%d,%d]\n",get_major(filp),get_minor(filp));
           *off += the_object->hi_valid_bytes;
           ret = copy_from_user(&(the_object->hi_prio_stream[*off]),buff,len);
           *off += (len - ret);
           the_object->hi_valid_bytes = *off;
           PDEBUG("after write HIGH LEVEL STREAM : %s \n", the_object->hi_prio_stream);
           mutex_unlock(&(the_object->mutex_hi)); 
         }
         return len-ret;
  write_low :
    deferred_work_t *data = kzalloc(sizeof(deferred_work_t),GFP_ATOMIC);
    bool result;
    data->bff=kzalloc(sizeof(char)*len,GFP_ATOMIC);
    ret = copy_from_user(data->bff, buff, len);
    data->off=*off;
    data->filp=filp;
    data->len=len;
    INIT_WORK (&data->w, deferred_work); 
    result = queue_work(the_object->wq, &data->w);
		if (result == false)
			PERR("%s:  failed to queue_work\n",__func__);
    return 0;
}

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off) {

  int minor = get_minor(filp);
  int ret;
  object_state *the_object;

  the_object = objects + minor;

   if (the_object->op==0) { //non blocking operation 
     if(the_object->prio==0 ) {
           if(mutex_trylock(&the_object->mutex_hi)) { 
          return -EAGAIN;// return to the caller   
      } else {
          goto read_hi;
      }
     } else {
         if(mutex_trylock(&the_object->mutex_low)) { 
          return -EAGAIN;// return to the caller   
      } else {
          goto read_low;
      }
     }       
  } else { //blocking operation
      if (the_object->prio == 0) { //high priority stream  
        if (schedule_timeout(the_object->jiffies) == 0) { // if timeout expired - remove from wait queue
               PINFO("%s - command timed out.", __func__);
               return ETIMEDOUT;
        }
        if(mutex_lock_interruptible(&the_object->mutex_hi)) {
          return -ERESTARTSYS;
        } else {
          goto read_hi;
        }
      }
      else { //low priority stream
       if (schedule_timeout(the_object->jiffies) == 0) { // if timeout expired - remove from wait queue
               PINFO("%s - command timed out.", __func__);
               return ETIMEDOUT;
        }    
        if(mutex_lock_interruptible(&the_object->mutex_low)) {
          return ERESTARTSYS;
        } else {
              goto read_low;
        }
       }
      }
  read_hi : 
    if(*off > the_object->hi_valid_bytes) {
            	mutex_unlock(&(the_object->mutex_hi));
	            return 0;
        } 
        if((the_object->hi_valid_bytes - *off) < len) len = the_object->hi_valid_bytes - *off; {
         PDEBUG("current HIGH LEVEL STREAM : %s \n", the_object->hi_prio_stream);
         PINFO("somebody called a high-prio read on dev with [major,minor] number [%d,%d]\n",get_major(filp),get_minor(filp));
         ret = copy_to_user(buff,&(the_object->hi_prio_stream[*off]),len);
         off += (len - ret);
         the_object->hi_prio_stream+=len;
         the_object->hi_valid_bytes-=len;
         PDEBUG("after read HIGH LEVEL STREAM : %s \n", the_object->hi_prio_stream);
         mutex_unlock(&(the_object->mutex_hi)); 
         return len - ret;
         }
  read_low: 
      if(*off > the_object->low_valid_bytes) {
            	 mutex_unlock(&(the_object->mutex_low));
	             return 0;
        } 
        if((the_object->low_valid_bytes - *off) < len) len = the_object->low_valid_bytes - *off; {
         PDEBUG("current LOW LEVEL STREAM : %s \n", the_object->low_prio_stream);
         PINFO("somebody called a low-prio read on dev with [major,minor] number [%d,%d]\n",get_major(filp),get_minor(filp));
         ret = copy_to_user(buff,&(the_object->low_prio_stream[*off]),len);
         off += (len - ret);
         the_object->low_prio_stream+=len;
         the_object->low_valid_bytes-=len;
         PDEBUG("after read LOW LEVEL STREAM : %s \n", the_object->low_prio_stream);
         mutex_unlock(&(the_object->mutex_low)); 
         return len - ret;
       }
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
    	the_object->TIMEOUT= arg; //milliseconds
      the_object->jiffies=msecs_to_jiffies(the_object->TIMEOUT);
      PINFO("Set timeout milliseconds: %d // Set timeout jiffies %ld\n", the_object->TIMEOUT, the_object->jiffies);
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



static int __init multiflowdriver_init(void) {
int i;
	//initialize the drive internal state
	for(i=0;i<MINORS;i++){
		objects[i].hi_valid_bytes = 0;
		objects[i].low_valid_bytes = 0;
		objects[i].hi_prio_stream = NULL;
		objects[i].low_prio_stream = NULL;
		objects[i].hi_prio_stream = (char*)__get_free_page(GFP_KERNEL);
		objects[i].low_prio_stream = (char*)__get_free_page(GFP_KERNEL);
   	mutex_init(&(objects[i].mutex_low));
    mutex_init(&(objects[i].mutex_hi));
    init_waitqueue_head(&(objects[i].hi_queue));
    init_waitqueue_head(&(objects[i].low_queue));
    ///initialize workqueue
    /* max_active is 0, which means set as 256 for max_active. */
    char str[15];
    sprintf( str, "%s%d", "mfdev_wq_", i );
    objects[i].wq  = alloc_workqueue(str, WQ_HIGHPRI | WQ_UNBOUND , 0);
		if(objects[i].hi_prio_stream == NULL || objects[i].low_prio_stream ==NULL ) goto revert_allocation;
	}

	Major = __register_chrdev(0, 0, 128, DEVICE_NAME, &fops);
	//actually allowed minors are directly controlled within this driver
	if (Major < 0) {
	  printk("registering device failed\n");
	  return Major;
	}
  
  //
	PINFO("new device registered, it is assigned major number %d\n", Major);
	return 0;
  revert_allocation:
	for(;i>=0;i--){
		free_page((unsigned long)objects[i].hi_prio_stream);
		free_page((unsigned long)objects[i].low_prio_stream);
	}
	return -ENOMEM;


}

static void __exit multiflowdriver_exit(void) {

	int i;
	for(i=0;i<MINORS;i++){
		free_page((unsigned long)objects[i].low_prio_stream);
		free_page((unsigned long)objects[i].hi_prio_stream);
    destroy_workqueue(objects[i].wq);
	  kfree(objects[i].wq);
	}
  
	
	unregister_chrdev(Major, DEVICE_NAME);

	PINFO("%s: new device unregistered, it was assigned major number %d\n", Major);

	return;

}
module_init(multiflowdriver_init);
module_exit(multiflowdriver_exit);

//module_param(enabled, int, S_IRUGO);

//module_param(nthreads, int, S_IRUGO);

//module_param(nbytes, int, S_IRUGO);

