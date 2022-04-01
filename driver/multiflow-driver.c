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
static long dev_ioctl(struct file *, unsigned int , unsigned long );
static ssize_t dev_read(struct file *, char *, size_t , loff_t *);
//deferred work 
static void deferred_work(struct work_struct *);

void call_deferred_work(int, char **, int, int, struct file **);
void sync_read(int , char **  , char ** , struct mutex * , wait_queue_head_t *, int *);
void sync_write(int , char **  , char ** , struct mutex * , wait_queue_head_t *, int *) ;
//DEVICE DRIVER TABLE - OPERATIONS
static struct file_operations fops = {
  .owner = THIS_MODULE,
  .write = dev_write,
  .read = dev_read,
  .open =  dev_open,
  .release = dev_release,
  .unlocked_ioctl = dev_ioctl
};

//DEFERRED WORK FUNCTION
static void deferred_work(struct work_struct *work) {
   int minor;
   device *dev;
   deferred_work_t *tw;
   tw = container_of(work, deferred_work_t, w);
   minor = get_minor(tw->filp);
   dev = devices + minor;   
   mutex_lock(&dev->mutex_low);
   PINFO("[Deferred work]=> PID: %d; NAME: %s\n", current->pid, current->comm);

   PDEBUG("[Deferred work]=>before write : %s\n", dev->low_prio_stream);
   (tw->off)  = dev->low_valid_bytes;

   sync_write((tw->len),&(dev->low_prio_stream),&(tw->bff),&(dev->mutex_low),&(dev->low_queue),&(dev->low_valid_bytes));
  
   PDEBUG("[Deferred work]=>after write : %s\n", dev->low_prio_stream);
   kfree(tw);   
 }
 
//OPEN
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
      //allocate new session with default parameters
      session->TIMEOUT=DEFAULT_TIMEOUT;
      session->op=DEFAULT_OP;
      session->prio=DEFAULT_PRIORITY;
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

//RELEASE
static int dev_release(struct inode *inode, struct file *file) {

   int minor;
   session_data_t *session;

   minor = get_minor(file);
   session = file->private_data;
   kfree(session);

   PINFO("device file closed\n");
   return 0;
}

//WRITE
static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

  int minor ;
  int ret;
  device *dev;
  session_data_t *session;
  char * tmp_buff ;

  minor = get_minor(filp);
  dev = devices + minor;
  session = filp->private_data;
  ////////////////////////////////////////////////////////////
  //save to temporary buffer before write to the real stream 
  tmp_buff = kzalloc(sizeof(char)*len,GFP_KERNEL);
  memset(tmp_buff,0,len);
  ret = copy_from_user(tmp_buff, buff, len);
  
  //gestire il fallimento della copy from user
  if(ret>0) { //partial copy

  }
  PINFO("[write]=>request to write %s\n",tmp_buff);
  //////////////////////////////////////////////////////////
  //OPERATION  
   if (session->prio == 0) { //high priority flow 
     if (session->op==0) { //non blocking operation 
      if(!mutex_trylock(&dev->mutex_hi)) {
         PERR("[Non-Blocking write]=> PID: %d; NAME: %s - RESOURCE BUSY\n", current->pid, current->comm);
         return 0;
      } 
     }  else { //blocking operation
        __atomic_fetch_add(&high_waiting[minor], 1, __ATOMIC_SEQ_CST);
        if (!wait_event_timeout(dev->hi_queue, mutex_trylock(&(dev->mutex_hi)), session->jiffies)) {
           PINFO("%s - command timed out - \n", __func__);
           __atomic_fetch_sub(&high_waiting[minor], 1, __ATOMIC_SEQ_CST);
           return 0;
        } else {
           __atomic_fetch_sub(&high_waiting[minor], 1, __ATOMIC_SEQ_CST);
        }
      }
    PINFO("[write/hi_prio_stream]=>stream before write %s\n",dev->hi_prio_stream);
    sync_write(len,&(dev->hi_prio_stream), &(tmp_buff),&(dev->mutex_hi),&(dev->hi_queue),&(dev->hi_valid_bytes));
    PINFO("[write/hi_prio_stream]=>stream after write %s\n",dev->hi_prio_stream);
    }
  else { //low priority stream
        call_deferred_work(minor,&tmp_buff,len,dev->low_valid_bytes,&filp);
      }
return len-ret;
}


void  call_deferred_work(int minor, char ** buff, int len, int off, struct file **filp) {
       device *dev;
       deferred_work_t *data;

       dev = devices + minor;
       //allocate new deferred_work item
       data = kzalloc(sizeof(deferred_work_t),GFP_KERNEL); //create the deferred_work struct for the deferred work
       data->bff=*buff;
       data->off=off;
       data->filp=*filp;
       data->len=len;
       //init work 
       INIT_WORK(&data->w, deferred_work);  
       queue_work(dev->wq, &data->w); //enqueue the deferred work
}


void sync_write(int len, char ** stream , char ** buff, struct mutex * mtx, wait_queue_head_t *wq, int *valid) {
   
   //allocate new memory
   *stream = krealloc(*stream,(*valid+len),GFP_ATOMIC);
   //clear new memory
   memset(*stream+*valid ,0,len);
   //concatenate the buff to the stream
   strncat(*stream,*buff ,len);
   mutex_unlock(mtx); 
   //update len 
   *valid+=len;
   wake_up(wq);
}

void sync_read(int len, char ** stream , char ** tmp_buff, struct mutex * mtx, wait_queue_head_t *wq, int *valid) {

        if(len > *valid) { //IF REQUEST BYTES TO READ ARE MAJOR TO THE VALID BYTES.. READ ONLY THE VALID BYTES..
              len=len-(len-*valid);
        }
         //copy first len bytes to tmp buff
         memmove(*tmp_buff, *stream,len);
         //clear after reading 
         memmove(*stream, *stream + len,*valid-len); //shift
         memset(*stream+ *valid - len,0,len); //clear
         *stream = krealloc(*stream,*valid - len,GFP_ATOMIC);
         //resettig parameters 
         *valid -= len;
         mutex_unlock(mtx); 
         wake_up(wq);
}

//READ
static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off) {
  int minor;
  int ret;
  device *dev;
  session_data_t *session;
  char * tmp_buff;

  minor = get_minor(filp);
  dev = devices + minor;
  session = filp->private_data;
  tmp_buff = kzalloc(sizeof(char)*len,GFP_ATOMIC);
  memset(tmp_buff,0,len); //clear temporary buffer

  if (session->op==0) { //non blocking operation 
     if(session->prio==0 ) {  //high priority stream 
           if(!mutex_trylock(&dev->mutex_hi)) {  //try ONCE to get the lock, not blocking
              PERR("[Non-Blocking read]=> PID: %d; NAME: %s - CAN'T DO THE OPERATION\n", current->pid, current->comm);
              return 0;// return to the caller   
            } else {
          sync_read(len,&(dev->hi_prio_stream),&(tmp_buff),&(dev->mutex_hi),&(dev->hi_queue),&(dev->hi_valid_bytes));
             }
     } else {
         if(!mutex_trylock(&dev->mutex_low)) { //try ONCE to get the lock, not blocking
          PERR("[Non-Blocking read]=> PID: %d; NAME: %s - CAN'T DO THE OPERATION\n", current->pid, current->comm);
          return 0;// return to the caller   
      } else {
         sync_read(len,&(dev->low_prio_stream),&(tmp_buff),&(dev->mutex_low),&(dev->low_queue),&(dev->low_valid_bytes));
      }
     }       
  } else { //blocking operation 
      if (session->prio == 0) { //high priority stream  
        __atomic_fetch_add(&high_waiting[minor], 1, __ATOMIC_SEQ_CST); 
        if (!wait_event_timeout(dev->hi_queue, mutex_trylock(&(dev->mutex_hi)), session->jiffies)) { //TIMEOUT!!
           PINFO("%s - command timed out - \n", __func__); //TIMEOUT EXPIRED
          __atomic_fetch_sub(&high_waiting[minor], 1, __ATOMIC_SEQ_CST);
           return 0;
        }
        else {
          __atomic_fetch_sub(&high_waiting[minor], 1, __ATOMIC_SEQ_CST);
          PINFO("[read/hi_prio_stream]=>stream before read %s\n",dev->hi_prio_stream);
          sync_read(len,&(dev->hi_prio_stream),&(tmp_buff),&(dev->mutex_hi),&(dev->hi_queue),&(dev->hi_valid_bytes));
          PINFO("[read/hi_prio_stream]=>stream after read %s\n",dev->hi_prio_stream);
        }
      }
      else { //low priority stream
         __atomic_fetch_add(&low_waiting[minor], 1, __ATOMIC_SEQ_CST); 
        if (!wait_event_timeout(dev->low_queue, mutex_trylock(&(dev->mutex_low)), session->jiffies)) { //waiting until the condition if true OR timeout expired 
          __atomic_fetch_sub(&low_waiting[minor], 1, __ATOMIC_SEQ_CST);
            PINFO("%s - command timed out - \n", __func__); //TIMEOUT EXPIRED
            return 0;
        } else {
         __atomic_fetch_sub(&low_waiting[minor], 1, __ATOMIC_SEQ_CST); 
         PINFO("[read/low_prio_stream]=> stream before read %s\n",dev->low_prio_stream);
         sync_read(len,&(dev->low_prio_stream),&(tmp_buff),&(dev->mutex_low),&(dev->low_queue),&(dev->low_valid_bytes));
         PINFO("[read/low_prio_stream]=> stream after read %s\n",dev->low_prio_stream);
        }
       }
      }
//finally, copy to user
ret = copy_to_user(buff,tmp_buff,len);
kfree(tmp_buff);

return len-ret;
}



//IOCTL
static long dev_ioctl(struct file *filp, unsigned int command, unsigned long arg) {

  int minor;
  int major;
  device *dev;
  session_data_t *session;

  minor= get_minor(filp);
  major = get_major(filp);
  dev = devices + minor;
  session = filp->private_data;

  switch (command) {

    case IOCTL_RESET :
      session->prio=DEFAULT_PRIORITY;
      session->op=DEFAULT_OP;
      session->TIMEOUT=DEFAULT_TIMEOUT;
      session->jiffies=((DEFAULT_TIMEOUT*HZ)/1000);
      PINFO("CALLED IOCTL_RESET ON[MAJ-%d,MIN-%d]\n ",major, minor);
      break;
		case IOCTL_HIGH_PRIO:
      PINFO("CALLED IOCTL_HIGH_PRIO ON[MAJ-%d,MIN-%d] \n ",major, minor);
    	session->prio = 0;
			break;
		case IOCTL_LOW_PRIO:
      PINFO("CALLED IOCTL_LOW_PRIO ON[MAJ-%d,MIN-%d] \n",major, minor);
    	session->prio = 1;
			break;
    case IOCTL_BLOCKING :
      PINFO("CALLED IOCTL_BLOCKING ON[MAJ-%d,MIN-%d]  \n",major, minor);
      session->op = 1;
			break;
    case IOCTL_NO_BLOCKING:
      PINFO("CALLED IOCTL_NO_BLOCKING ON[MAJ-%d,MIN-%d]  \n",major, minor);
     	session->op = 0;
			break;
    case IOCTL_SETTIMER :
      if ((int) arg > MAX_TIMEOUT) {
           session->TIMEOUT= MAX_TIMEOUT; 
           session->jiffies=msecs_to_jiffies(MAX_TIMEOUT);
        }
      if ((int) arg <= 0) {  
            session->TIMEOUT= DEFAULT_TIMEOUT; //milliseconds
            session->jiffies=msecs_to_jiffies(DEFAULT_TIMEOUT);
        }
      else {
        session->TIMEOUT=(int)arg; //milliseconds
        session->jiffies=msecs_to_jiffies(arg);
      }
      PINFO("CALLED IOCTL_SETTIMER ON[MAJ-%d,MIN-%d] : TIMEOUT SET %lu [HZ] \n", major, minor, session->jiffies);
      break;
    case IOCTL_ENABLE:
     	devices_state[minor]=0;
      PINFO("CALLED IOCTL_ENABLE ON[MAJ-%d,MIN-%d] \n ",major, minor);
			break;  
    case IOCTL_DISABLE:
     	devices_state[minor]=1;
      PINFO("CALLED IOCTL_DISABLE ON[MAJ-%d,MIN-%d] \n ",major, minor);
			break;    
    case IOCTL_TIMER_TEST: //ONLY FOR TEST PURPOSE!!!!! ..
        session->jiffies= nsecs_to_jiffies(1);
        PINFO("CALLED IOCTL_SETTIMER ON[MAJ-%d,MIN-%d] : TIMEOUT SET %lu [HZ]\n ", major, minor, session->jiffies);
        break;
		default:
      PERR("UNKOWN IOCTL COMMAND\n");
			return -ENOTTY;
  }
  return 0;
}


//INIT
static int __init multiflowdriver_init(void) {
int i;
char str[15];
	//initialize the drive internal state
	for(i=0;i<MINORS;i++){
		devices[i].hi_valid_bytes = 0;
		devices[i].low_valid_bytes = 0;
		devices[i].hi_prio_stream = NULL;  //dynamic memory
		devices[i].low_prio_stream = NULL;  //dynamic memory 
   	mutex_init(&(devices[i].mutex_low));
    mutex_init(&(devices[i].mutex_hi));
    init_waitqueue_head(&(devices[i].hi_queue)); //init the waitqueue
    init_waitqueue_head(&(devices[i].low_queue));
    ///initialize workqueue
    memset(str,0,15);
    sprintf( str, "%s%d", "mfdev_wq_", i );
    devices[i].wq  = alloc_workqueue(str, WQ_HIGHPRI | WQ_UNBOUND , 0);  //allocate workqueue for devices[i]
  }
  
	Major = __register_chrdev(0, 0, 128, DEVICE_NAME, &fops);
	//actually allowed minors are directly controlled within this driver
  
	if (Major < 0) {
	  printk("registering device failed\n");
	  return Major;
	} 

	PINFO("new device registered, it is assigned major number %u\n", Major);
	return 0;

}

//EXIT
static void __exit multiflowdriver_exit(void) {

	int i;
	for(i=0;i<MINORS;i++){
    flush_workqueue(devices[i].wq);
    destroy_workqueue(devices[i].wq);
    kfree(devices[i].low_prio_stream);
    kfree(devices[i].hi_prio_stream);
	}  
	unregister_chrdev(Major, DEVICE_NAME);
	PINFO("new device unregistered, it was assigned major number %u\n", Major);
	return;

}

module_init(multiflowdriver_init);
module_exit(multiflowdriver_exit);

module_param_array(devices_state,int,NULL,S_IWUSR|S_IRUSR); //Devices state (0 = enabled - 1 = disable)
module_param_array(high_bytes,int,NULL,S_IRUGO);   //# valid bytes are present in every high priority flow
module_param_array(low_bytes,int,NULL,S_IRUGO); //#valid bytes are present in every low priority flow
module_param_array(high_waiting, int,NULL,S_IRUGO);   //#threads waiting on high priority stream for every device
module_param_array(low_waiting, int,NULL,S_IRUGO); //#threads are waiting on low priority stream for every device

MODULE_PARM_DESC(devices_state, "Array of devices states (0 = enabled - 1 = disabled)");
MODULE_PARM_DESC(high_bytes, "Array reporting the number of current valid bytes in the high priority stream of every device.");
MODULE_PARM_DESC(low_bytes, "Array reporting the number of current valid bytes in the low priority stream of every device.");
MODULE_PARM_DESC(high_waiting, "Array describing the number of threads waiting on the high priority stream of every device.");
MODULE_PARM_DESC(low_waiting, "Array describing the number of threads waiting on the low priority stream of every device.");
