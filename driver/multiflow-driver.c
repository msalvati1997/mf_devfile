/*
=====================================================================================================
Driver Name		:		multiflow-driver
Author			:		MARTINA SALVATI
License			:		GPL
Description		:		LINUX MULTI_FLOW DEVICE DRIVER 
=====================================================================================================
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

/**
 * @brief  Devive's driver table
 * 
 */
static struct file_operations fops = {
  .owner = THIS_MODULE,
  .write = dev_write,
  .read = dev_read,
  .open =  dev_open,
  .release = dev_release,
  .unlocked_ioctl = dev_ioctl
};

//deferred work 
static void deferred_work(struct work_struct *);

//functional methods
void call_deferred_work(int, char **, int, int, struct file **);
void sync_read(int , char **  , char ** , struct mutex * , wait_queue_head_t *, int *, int*);
void sync_write(int , char **  , char ** , struct mutex * , wait_queue_head_t *, int *, int *);
int dev_lock(session_data_t *, struct mutex *, wait_queue_head_t *, int *);


/**
 * @brief This is the delayed work function. Write to the low priority stream.
 *        The function never fails.
 * 
 * @param work  work contained in __deferred_work_item 's struct
 */
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

   sync_write((tw->len),&(dev->low_prio_stream),&(tw->bff),&(dev->mutex_low),&(dev->low_queue),&(dev->low_valid_bytes),&low_bytes[minor]);
  
   PDEBUG("[Deferred work]=>after write : %s\n", dev->low_prio_stream);
   

   kfree(tw);   
 }

/**
 * @brief This is the function that performs the setup of the structure to be sent to the delayed work function.
 * 
 * @param minor minor of device
 * @param buff  buffer to write
 * @param len   number of bytes to write
 * @param off   offset 
 * @param filp  pointer to Struct file
 */
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

/**
 * @brief This is the function that writes to one of the two streams of the device. 
 * 
 * @param len         number of bytes to writes
 * @param stream      stream's device to write to 
 * @param buff        nuffer to write to the stream
 * @param mtx         synchronizer
 * @param wq          pointer to wait_queue_head
 * @param valid       pointer to dev -> valid_param (high/low)
 * @param param_bytes pointer to the module's param about the valid bytes of the stream
 * 
 */
void sync_write(int len, char ** stream , char ** buff, struct mutex * mtx, wait_queue_head_t *wq, int *valid, int *param_bytes) {
   
   //allocate new memory
   *stream = krealloc(*stream,(*valid+len),GFP_ATOMIC);
   //clear new memory
   memset(*stream+*valid ,0,len);
   //concatenate the buff to the stream
   strncat(*stream,*buff ,len);
   //update param 
   *valid+=len;
   *param_bytes=*valid;
   
   mutex_unlock(mtx); 
   wake_up(wq);

   kfree(*buff);
}
/**
 * @brief This is a function that does two things:
            (1) fetching the bytes requested by the user's request read
            (2) removing of data collected by the user from one of the two streams in FIFO's mode.
 * 
 * @param len         number of bytes to fetch
 * @param stream      stream to read
 * @param tmp_buff    stream to pass to the user with copy_to_user
 * @param mtx         synchronizer
 * @param wq          pointer to the wait_queue_head_t (high/low)
 * @param valid       pointer to dev -> valid_param (high/low)
 * @param param_bytes pointer to the module's param about the valid bytes of the stream
 **/
void sync_read(int len, char ** stream , char ** tmp_buff, struct mutex * mtx, wait_queue_head_t *wq, int *valid, int *param_bytes) {

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
         *param_bytes = *valid;
         mutex_unlock(mtx); 
         wake_up(wq);
}
/**
 * @brief  This function is about acquiring the lock.
 *         The acquisition of the lock depends on the type of operation. 
 *         If non-blocking, try to take the lock once and then return to the caller. 
 *         If blocking, the wait_event_timeout API is used. 
 *         In the blocking case the thread is blocked as long as :
 *              (1) the condition is true (the lock is taken) 
 *              (2) the timeout is expired.
 * 
 * @param session           pointer to the session struct
 * @param mtx               pointer to the mutex struct
 * @param wq                pointer to the waitqueue struct
 * @param param_wait        pointer to the param of waiting thread 
 * @return int              0 fail, 1 success
 */
int dev_lock(session_data_t *session, struct mutex * mtx, wait_queue_head_t *wq, int *param_wait) {

     if(session->op==0) //non blocking 
     {
        if(!mutex_trylock(mtx)) {  //try ONCE to get the lock, not blocking
              PERR("[Non-Blocking op]=> PID: %d; NAME: %s - CAN'T DO THE OPERATION\n", current->pid, current->comm);
              return 0;// return to the caller   
            } 
          else {
            return 1;
          }
      } else { //blocking
           __atomic_fetch_add(param_wait, 1, __ATOMIC_SEQ_CST); 
        if (!wait_event_timeout(*wq, mutex_trylock(mtx), session->jiffies)) { //TIMEOUT!!
           PERR("[Blocking op]=> PID: %d; NAME: %s - TIMEOUT EXPIRED\n", current ->pid, current->comm); //TIMEOUT EXPIRED
          __atomic_fetch_sub(param_wait, 1, __ATOMIC_SEQ_CST);
           return 0;
        }
        else {
          __atomic_fetch_sub(param_wait, 1, __ATOMIC_SEQ_CST);
          return 1; 
        }
      } 
 
}

/*
=====================================================================================================
                                      DRIVER OPERATIONS
=====================================================================================================
*/
 
/**
 * @brief  The device opening operation allocates a private session structure. 
 *         The allocation of the structure is allowed only if the device_state of the object is set to ENABLE, 
 *         otherwise (DISABLE) it is not possible to create new sessions.
 * 
 * @param inode    pointer to struct inode 
 * @param file     pointer to struct file
 * @return int     return 0 or errno is set to a correct value
 */
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

/**
 * @brief  This operation is used to close a specific device and to deallocate the session's private data.
 * 
 * @param inode  pointer to struct inode
 * @param file   pointer to struct file
 * @return int   return 0 on success
 */
static int dev_release(struct inode *inode, struct file *file) {

   int minor;
   session_data_t *session;

   minor = get_minor(file);
   session = file->private_data;
   kfree(session);

   PINFO("device file closed\n");
   return 0;
}

/**
 * @brief  This is the driver write operation. The write operation of n bytes involves the allocation of n bytes of memory. 
 *         The write operation has a different behavior for each session's setting parameters : priority (high/low), 
 *         operations' mode (blocking/non blockign).
 * 
 * @param filp     pointer to struct file 
 * @param buff     userspace's buffer that contains the request's bytes to write to the device's stream
 * @param len      number of bytes to write to the device's stream
 * @param off      offset 
 * 
 * @return ssize_t number of written bytes
 */
static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

  int minor ;
  int ret;
  device *dev;
  session_data_t *session;
  char * buff_tmp ;

  minor = get_minor(filp);
  dev = devices + minor;
  session = filp->private_data;
  
  //save to temporary buffer before write to the real stream 
  buff_tmp = kzalloc(sizeof(char)*len,GFP_KERNEL);
  memset(buff_tmp,0,len);
  ret = copy_from_user(buff_tmp, buff, len);
  PINFO("[write]=>request to write %s\n",buff_tmp);
  
  //OPERATION  
   if (session->prio == 0) { //high priority flow 
            if(dev_lock(session,&dev->mutex_hi,&dev->hi_queue,&high_waiting[minor])==1) { //acquiring the lock, depends on the type of operation in session data
               PINFO("[write/hi_prio_stream]=>stream before write %s\n",dev->hi_prio_stream);
               sync_write(len,&(dev->hi_prio_stream), &(buff_tmp),&(dev->mutex_hi),&(dev->hi_queue),&(dev->hi_valid_bytes),&high_bytes[minor]); 
               PINFO("[write/hi_prio_stream]=>stream after write %s\n",dev->hi_prio_stream);    
            } else {
              return 0;
            }
    }
  else { //low priority stream
        call_deferred_work(minor,&buff_tmp,len,dev->low_valid_bytes,&filp);
      }


return len-ret;
}


/**
 * @brief  This is the driver read operation. Readings are performed in FIFO mode from left to right. 
 *         When a read is performed then the readed bytes are removed from the stream.
 * 
 * @param filp      pointer to struct file 
 * @param buff      userspace's buffer that will contains the  bytes requested from user to read
 * @param len       number of bytes to read 
 * @param off       offset
 * 
 * @return ssize_t  number of readed bytes
 */

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

  if (session->prio==0)   { //high priority stream
    if(dev_lock(session,&dev->mutex_hi,&dev->hi_queue,&high_waiting[minor])==1) {
        PINFO("[read/hi_prio_stream]=>stream before read %s\n",dev->hi_prio_stream);
        sync_read(len,&(dev->hi_prio_stream),&(tmp_buff),&(dev->mutex_hi),&(dev->hi_queue),&(dev->hi_valid_bytes),&(high_bytes[minor]));
        PINFO("[read/hi_prio_stream]=>stream after read %s\n",dev->hi_prio_stream);
    } else {
      return 0;
    }
  } else { //low priority stream
    if(dev_lock(session,&dev->mutex_low,&dev->low_queue,&low_waiting[minor])==1) {
         PINFO("[read/low_prio_stream]=> stream before read %s\n",dev->low_prio_stream);
         sync_read(len,&(dev->low_prio_stream),&(tmp_buff),&(dev->mutex_low),&(dev->low_queue),&(dev->low_valid_bytes),&(low_bytes[minor]));
         PINFO("[read/low_prio_stream]=> stream after read %s\n",dev->low_prio_stream);
    } else {
       return 0;
    }
  }

//finally, copy to user
ret = copy_to_user(buff,tmp_buff,len);
kfree(tmp_buff);

return len-ret;
}



/**
 * @brief   This function handles all "interface"-type I/O control requests. 
 * 
 * @param filp  pointer to struct file 
 * @param command   
 *              The possible commands to call : 
 *              =====================================================================================================
 *              IOCTL_RESET 	      Allows to set the default setting parameters of the device file.
 *              IOCTL_HIGH_PRIO   	Allows to set the workflow to high priority.
 *              IOCTL_LOW_PRIO 	    Allows to set the workflow to low priority.
 *              IOCTL_BLOCKING 	    Allows to set the working mode operation to non blocking.
 *              IOCTL_NO_BLOCKING 	Allows to set the working mode operation to blocking.
 *              IOCTL_SETTIMER    	Allows to set the timer for blocking operation.
 *              IOCTL_ENABLE 	      Allows to set the device state to enable.
 *              IOCTL_DISABLE    	  Allows to set the device state to disable.
 *              IOCTL_TIMER_TEST    Only for test purpose. Set timeout to nsecs.
 *              =====================================================================================================
 * 
 * @param arg unsined long in user space (timeout's var)
 * 
 * @return long  The return value is the return from the syscall if
 *	             positive or a negative errno code on error.
 */
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

/*
=====================================================================================================
                                   MODULE INIT & EXIT 
=====================================================================================================
*/

/**
 * @brief  Initialize the module with all needed structures.
 * 
 * @return 0 or a negative errno code on error. 
 */
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

	PINFO("New device registered, it is assigned major number %u\n", Major);
	return 0;

}

/**
 * @brief Module Cleanup. All driver data structures are deallocated.
 * 
 */
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

MODULE_PARM_DESC(devices_state, "This param set the state of the device file. If set to enable, new session can be created. If set to disable, new session can't be created.");
MODULE_PARM_DESC(low_waiting, "This param indicates the thread currently waiting on the waitqueue reserved to the process working to the low stream.");
MODULE_PARM_DESC(high_waiting, "This param indicates the thread currently waiting on the waitqueue reserved to the process working to the hi stream.");
MODULE_PARM_DESC(high_bytes, "This param indicates the number of bytes present on the high prio stream");
MODULE_PARM_DESC(low_bytes, "This param indicates the number of bytes present on the low prio stream");


