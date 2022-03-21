/*
===============================================================================
Driver Name		:		multiflow-driver
Author			:		MARTINA SALVATI
License			:		GPL
Description		:		LINUX MULTI_FLOW DEVICE DRIVER 
===============================================================================
*/

#define EXPORT_SYMTAB
#include "multiflow-driver.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martina Salvati");
MODULE_DESCRIPTION("Multi-flow device file.");

//driver operations
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static long dev_ioctl(struct file *filp, unsigned int command, unsigned long arg);
//deferred work 
static void deferred_work(struct work_struct *work);


static void deferred_work(struct work_struct *work) {
   int minor;
   device *dev;
   PINFO("[Deferred work]=> PID: %d; NAME: %s\n", current->pid, current->comm);
   deferred_work_t *tw;
   tw = container_of(work, deferred_work_t, w);
   if (!tw) {
		PERR("%s: Null pointer!!\n", __func__);
    return;
 	  }
   minor = get_minor(tw->filp);
   dev = devices + minor;
   session_data_t *session = (tw->filp)->private_data;


if (session->op==0) { //non blocking operation
        if(!mutex_trylock(&dev->mutex_low)) { //non blocking 
         PERR("[Non-Blocking op]=> PID: %d; NAME: %s - CAN'T DO THE OPERATION\n", current->pid, current->comm);
         return;
        } else {
          goto deferred_write;
        } 
 } else { //blocking operation
     __atomic_fetch_add(&low_waiting[minor], 1, __ATOMIC_SEQ_CST);
      if(!wait_event_timeout(dev->low_queue, mutex_trylock(&(dev->mutex_low)), session->jiffies)) {
           PINFO("%s - command timed out - \n", __func__);
           __atomic_fetch_sub(&low_waiting[minor], 1, __ATOMIC_SEQ_CST);
           return;
        }
        else {
          __atomic_fetch_sub(&low_waiting[minor], 1, __ATOMIC_SEQ_CST);
          goto deferred_write;
        }
     }

  deferred_write: 
   if((tw->off) >= OBJECT_MAX_SIZE) {//offset too large
   	      PERR("offset too large\n");
          mutex_unlock(&dev->mutex_low);
          wake_up(&(dev->low_queue)); //wake up the waiting thread on the low prio queue
          kfree(tw);
          return;
        }
   if((tw->off) > dev->low_valid_bytes) {//offset bwyond the current stream size
    	   PERR("out of stream resources\n");
         mutex_unlock(&dev->mutex_low);
         wake_up(&(dev->low_queue)); //wake up the waiting thread on the low prio queue
         kfree(tw);
         return;
       }  
   if((OBJECT_MAX_SIZE - (tw->off)) < (tw->len)) (tw->len) = OBJECT_MAX_SIZE - (tw->off); {
        PDEBUG("before deferred-write LOW LEVEL STREAM : %s\n", dev->low_prio_stream);
        (tw->off)  = dev->low_valid_bytes;
        strncat(&(dev->low_prio_stream[(tw->off)]),(tw->bff) ,(tw->len));
        tw->off +=tw->len;
        dev->low_valid_bytes = (tw->off); 
        PDEBUG("after deferred-write LOW LEVEL STREAM : %s\n", dev->low_prio_stream);
        low_bytes[minor] = dev->low_valid_bytes;
        mutex_unlock(&(dev->mutex_low));
        wake_up(&(dev->low_queue)); //wake up the waiting thread on the low prio queue
        kfree(tw);
      }
 }
 

static int dev_open(struct inode *inode, struct file *file) {

   int minor;
   minor = get_minor(file);
   if(minor >= MINORS){
      PERR("Device open: internal error\n");
    	return -ENODEV;
   }
   if(devices_state[minor]==0) {  //if devices state is set to ENABLE 
      session_data_t *session = NULL;
      session= kzalloc(sizeof(session_data_t),GFP_ATOMIC); //allocate new session 
      if (session == NULL){
         PERR("Error allocating memory\n");
         return -ENOMEM;
      }
      //allocate new session with default timeout
      session->TIMEOUT=DEFAULT_TIMEOUT;
      session->op=DEFAULT_OP;
      session->prio=DEFAULT_PRIO;
      session->jiffies=msecs_to_jiffies(DEFAULT_TIMEOUT);
      file -> private_data = session; 
      PINFO("DEVICE FILE [MIN %d] OPENED :  NEW SESSION CREATED\n",minor);
      return 0;
   } else {
       //device set is set to DISABLE
      PERR("DEVICE FILE [MIN %d] DISABLED : CAN'T OPEN A NEW SESSION\n", minor);
      return 0;
   }
}


static int dev_release(struct inode *inode, struct file *file) {

   int minor;
   minor = get_minor(file);
   session_data_t *session = file->private_data;
   kfree(session);

   PINFO("device file closed\n");
   return 0;
}



static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

  int minor = get_minor(filp);
  int ret;
  device *dev;
  dev = devices + minor;
  session_data_t *session = filp->private_data;

  if (session->op==0) { //non blocking operation 
   if (session->prio == 0) { //high priority flow 
      if(!mutex_trylock(&dev->mutex_hi)) {
         PERR("[Non-Blocking write high prio]=> PID: %d; NAME: %s - RESOURCE BUSY\n", current->pid, current->comm);
         return -1;
      } else {
          goto write_hi;
      }
   } else {  //low priority flow
         goto write_low;
        } 
  } else { //blocking operation
      if (session->prio == 0) { //high priority stream 
           __atomic_fetch_add(&high_waiting[minor], 1, __ATOMIC_SEQ_CST);
        if(!wait_event_timeout(dev->hi_queue, mutex_trylock(&(dev->mutex_hi)), session->jiffies)) {
           PINFO("%s - command timed out - \n", __func__);
           __atomic_fetch_sub(&high_waiting[minor], 1, __ATOMIC_SEQ_CST);
           return -1;
        } else {
           __atomic_fetch_sub(&high_waiting[minor], 1, __ATOMIC_SEQ_CST);
          goto write_hi;
        }
      } else { //low priority stream
        goto write_low;   //deferred work
      }
  }

  write_hi:
        *off = dev->hi_valid_bytes; //set the offset at the end of stream of high prio
        if(*off >= OBJECT_MAX_SIZE) {//offset too large
     	      mutex_unlock(&(dev->mutex_hi));
            wake_up(&(dev->hi_queue));//wake up the waiting thread on the high prio stream
            PERR("No space left on device\n");
	          return -ENOSPC;//no space left on device
           }
        if(*off > dev->hi_valid_bytes) {//offset bwyond the current stream size
  	        mutex_unlock(&(dev->mutex_hi));
            wake_up(&(dev->hi_queue)); //wake up the waiting thread on the high prio stream
            PERR("Out of stream resources \n");
 	          return -ENOSR;
          }  
        if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off; {
           PINFO("somebody called a high-prio write on dev with [major,minor] number [%d,%d]\n",get_major(filp),get_minor(filp));
           PDEBUG("before write HIGH LEVEL STREAM : %s \n", dev->hi_prio_stream);
           ret = copy_from_user(&(dev->hi_prio_stream[*off]),buff,len);
           *off += (len - ret);
           dev->hi_valid_bytes = *off;           
           PDEBUG("after write HIGH LEVEL STREAM : %s \n", dev->hi_prio_stream);
           high_bytes[minor] = dev->hi_valid_bytes;
           mutex_unlock(&(dev->mutex_hi)); 
           wake_up(&(dev->hi_queue)); //wake up the waiting thread on the high prio stream
           return len-ret;
        }
        return 0;

        
  write_low: 
       *off = dev->low_valid_bytes; //set the offset at the end of stream of low prio
       deferred_work_t *data = kzalloc(sizeof(deferred_work_t),GFP_ATOMIC); //create the deferred_work struct for the deferred work
       bool result;
       data->bff=kzalloc(sizeof(char)*len,GFP_ATOMIC);
       ret = copy_from_user(data->bff, buff, len);
       data->off=*off;
       data->filp=filp;
       data->len=len;
       INIT_WORK(&data->w, deferred_work);  
       result = queue_work(dev->wq, &data->w); //enqueue the deferred work
 	  	 if (result == false)  {
			  PERR("%s:  failed to queue_work\n",__func__);
        return -1; 
        } 
        return 0;
  return 0;
}

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off) {
  int minor = get_minor(filp);
  int ret;
  device *dev;
  dev = devices + minor;
  int del_bytes;
  session_data_t *session = filp->private_data;

   if (session->op==0) { //non blocking operation 
     if(session->prio==0 ) {  //high priority stream 
           if(!mutex_trylock(&dev->mutex_hi)) {  //try ONCE to get the lock, not blocking
              PERR("[Non-Blocking op]=> PID: %d; NAME: %s - CAN'T DO THE OPERATION\n", current->pid, current->comm);
              return -1;// return to the caller   
      } else {
          goto read_hi;
      }
     } else {
         if(!mutex_trylock(&dev->mutex_low)) { //try ONCE to get the lock, not blocking
          PERR("[Non-Blocking op]=> PID: %d; NAME: %s - CAN'T DO THE OPERATION\n", current->pid, current->comm);
          return -1;// return to the caller   
      } else {
          goto read_low;
      }
     }       
  } else { //blocking operation 
      if (session->prio == 0) { //high priority stream  
        __atomic_fetch_add(&high_waiting[minor], 1, __ATOMIC_SEQ_CST); 
        if(!wait_event_timeout(dev->hi_queue, mutex_trylock(&(dev->mutex_hi)), session->jiffies)) { //waiting until the condition if true OR timeout EXPIRED
          wake_up(&(dev->hi_queue)); //Wake up the waiting thread on the high prio stream
           PINFO("%s - command timed out - \n", __func__); //TIMEOUT EXPIRED
          __atomic_fetch_sub(&high_waiting[minor], 1, __ATOMIC_SEQ_CST);
           return -1;
        }
        else {
          __atomic_fetch_sub(&high_waiting[minor], 1, __ATOMIC_SEQ_CST);
          goto read_hi;
        }
      }
      else { //low priority stream
         __atomic_fetch_add(&low_waiting[minor], 1, __ATOMIC_SEQ_CST); 
        if(!wait_event_timeout(dev->hi_queue, mutex_trylock(&(dev->mutex_hi)), session->jiffies)) { //waiting until the condition if true OR timeout expired 
           wake_up(&(dev->low_queue));  //wake up the waiting thread on the low prio stream 
          __atomic_fetch_sub(&low_waiting[minor], 1, __ATOMIC_SEQ_CST);
            PINFO("%s - command timed out - \n", __func__); //TIMEOUT EXPIRED
            return -1;
        } else {
         __atomic_fetch_sub(&low_waiting[minor], 1, __ATOMIC_SEQ_CST); 
         goto read_low;
        }
       }
      }
  read_hi :
        *off = dev->hi_valid_bytes; 
        if(len > dev->hi_valid_bytes) {
              len=len - (len-dev->hi_valid_bytes);
        } 
         PINFO("somebody called a high-prio read on dev with [major,minor] number [%d,%d]\n",get_major(filp),get_minor(filp));
         PDEBUG("before read  HIGH LEVEL STREAM : %s \n", dev->hi_prio_stream);
         ret = copy_to_user(buff,&(dev->hi_prio_stream[0]),len);
         del_bytes = len-ret;
         memmove(dev->hi_prio_stream, (dev->hi_prio_stream) + (del_bytes),(dev->hi_valid_bytes) - (del_bytes));
         memset(dev->hi_prio_stream + dev->hi_valid_bytes - del_bytes,0,del_bytes);
         *off = dev->hi_valid_bytes;
         *off -= del_bytes;
         dev->hi_valid_bytes = *off;
         high_bytes[minor] = dev->hi_valid_bytes;
         PDEBUG("after read HIGH LEVEL STREAM : %s \n", dev->hi_prio_stream);
         mutex_unlock(&(dev->mutex_hi)); 
         wake_up(&(dev->hi_queue));
         return len - ret;
         
  read_low: 
      *off = dev->low_valid_bytes;
       if(len > dev->low_valid_bytes) {
              len=len - (len -dev->low_valid_bytes);
        } 
         PINFO("somebody called a low-prio read on dev with [major,minor] number [%d,%d]\n",get_major(filp),get_minor(filp));
         PDEBUG("before read LOW LEVEL STREAM : %s \n", dev->low_prio_stream);
         ret = copy_to_user(buff,&(dev->low_prio_stream[0]),len);
         del_bytes = len-ret;
         memmove(dev->low_prio_stream, (dev->low_prio_stream) + (del_bytes),(dev->low_valid_bytes) - (del_bytes));
         memset(dev->low_prio_stream + dev->low_valid_bytes - del_bytes,0,del_bytes);
         *off = dev->low_valid_bytes;
         *off -= del_bytes;
         dev->low_valid_bytes = *off;
         low_bytes[minor] = dev->low_valid_bytes;
         PDEBUG("after read LOW LEVEL STREAM : %s \n", dev->low_prio_stream);
         mutex_unlock(&(dev->mutex_low)); 
         wake_up(&(dev->low_queue));
         return len - ret;
       
  }


static long dev_ioctl(struct file *filp, unsigned int command, unsigned long arg) {

  int minor = get_minor(filp);
  device *dev;
  dev = devices + minor;
  session_data_t *session = filp->private_data;
  switch (command) {

    case IOCTL_RESET :
      session->prio=DEFAULT_PRIO;
      session->op=DEFAULT_OP;
      session->TIMEOUT=DEFAULT_TIMEOUT;
      session->jiffies=msecs_to_jiffies(DEFAULT_TIMEOUT);
      PINFO("CALLED IOCTL_RESET ON[MAJ-%d,MIN-%d]  ",get_major(filp), get_minor(filp));
       break;
		case IOCTL_HIGH_PRIO:
      PINFO("CALLED IOCTL_HIGH_PRIO ON[MAJ-%d,MIN-%d]  ",get_major(filp), get_minor(filp));
    	session->prio = 0;
			break;
		case IOCTL_LOW_PRIO:
      PINFO("CALLED IOCTL_LOW_PRIO ON[MAJ-%d,MIN-%d]  ",get_major(filp), get_minor(filp));
    	session->prio = 1;
			break;
    case IOCTL_BLOCKING :
      PINFO("CALLED IOCTL_BLOCKING ON[MAJ-%d,MIN-%d]  ",get_major(filp), get_minor(filp));
      session->op = 1;
			break;
    case IOCTL_NO_BLOCKING:
      PINFO("CALLED IOCTL_NO_BLOCKING ON[MAJ-%d,MIN-%d]  ",get_major(filp), get_minor(filp));
     	session->op = 0;
			break;
    case IOCTL_SETTIMER :
      if ((int) arg < 0) {
        PERR("PROBLEM ON SETTING TIMEOUT VALUE\n");
      } else {
        session->TIMEOUT= (int) arg; //milliseconds
        session->jiffies=msecs_to_jiffies((int)arg);
        PINFO("CALLED IOCTL_SETTIMER ON[MAJ-%d,MIN-%d] : TIMEOUT SET %d [HZ] ", get_major(filp), get_minor(filp), session->jiffies);
      }
    case IOCTL_ENABLE:
     	devices_state[minor]=0;
      PINFO("CALLED IOCTL_ENABLE ON[MAJ-%d,MIN-%d]  ",get_major(filp), get_minor(filp));
			break;  
    case IOCTL_DISABLE:
     	devices_state[minor]=1;
      PINFO("CALLED IOCTL_DISABLE ON[MAJ-%d,MIN-%d]  ",get_major(filp), get_minor(filp));
			break;    
		default:
      PERR("UNKOWN IOCTL COMMAND\n");
			return -ENOTTY;
  }
  //do here whathever you would like to control the state of the device
  return 0;
}



static struct file_operations fops = {
  .owner = THIS_MODULE,
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
		devices[i].hi_valid_bytes = 0;
		devices[i].low_valid_bytes = 0;
		devices[i].hi_prio_stream = NULL;
		devices[i].low_prio_stream = NULL;
		devices[i].hi_prio_stream = (char*)__get_free_page(GFP_KERNEL);
		devices[i].low_prio_stream = (char*)__get_free_page(GFP_KERNEL);
   	mutex_init(&(devices[i].mutex_low));
    mutex_init(&(devices[i].mutex_hi));
    init_waitqueue_head(&(devices[i].hi_queue));
    init_waitqueue_head(&(devices[i].low_queue));
    ///initialize workqueue
    /* max_active is 0, which means set as 256 for max_active. */
    char str[15];
    sprintf( str, "%s%d", "mfdev_wq_", i );
    devices[i].wq  = alloc_workqueue(str, WQ_HIGHPRI | WQ_UNBOUND , 0);  //allocate workqueue for devices[i]
		if(devices[i].hi_prio_stream == NULL || devices[i].low_prio_stream ==NULL ) goto revert_allocation;
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
		free_page((unsigned long)devices[i].hi_prio_stream);
		free_page((unsigned long)devices[i].low_prio_stream);
    destroy_workqueue(devices[i].wq);
	 }
	return -ENOMEM;


}

static void __exit multiflowdriver_exit(void) {

	int i;
	for(i=0;i<MINORS;i++){
		free_page((unsigned long)devices[i].low_prio_stream);
		free_page((unsigned long)devices[i].hi_prio_stream);
    flush_workqueue(devices[i].wq);
    destroy_workqueue(devices[i].wq);
	}  
	
	unregister_chrdev(Major, DEVICE_NAME);

	PINFO("%s: new device unregistered, it was assigned major number %d\n", Major);

	return;

}

module_init(multiflowdriver_init);
module_exit(multiflowdriver_exit);

module_param_array(devices_state,int,NULL,S_IWUSR|S_IRUSR); //Devices state (0 = disabled - 1 = enabled)
module_param_array(high_bytes,int,NULL,S_IRUGO);   //# valid bytes are present in every high priority flow
module_param_array(low_bytes,int,NULL,S_IRUGO); //#valid bytes are present in every low priority flow
module_param_array(high_waiting, int,NULL,S_IRUGO);   //#threads waiting on high priority stream for every device
module_param_array(low_waiting, int,NULL,S_IRUGO); //#threads are waiting on low priority stream for every device

MODULE_PARM_DESC(devices_state, "Array of devices states (0 = enabled - 1 = disabled)");
MODULE_PARM_DESC(high_bytes, "Array reporting the number of current valid bytes in the high priority stream of every device.");
MODULE_PARM_DESC(low_bytes, "Array reporting the number of current valid bytes in the low priority stream of every device.");
MODULE_PARM_DESC(high_waiting, "Array describing the number of threads waiting on the high priority stream of every device.");
MODULE_PARM_DESC(low_waiting, "Array describing the number of threads waiting on the low priority stream of every device.");