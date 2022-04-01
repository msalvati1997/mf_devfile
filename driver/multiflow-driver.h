#include"multiflow-driver_ioctl.h"
#include<linux/capability.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/list.h>
#include<linux/fs.h>
#include<linux/string.h>
#include <linux/kref.h>
#include <linux/seq_file.h>
#include<linux/init.h>
#include<linux/ioctl.h>
#include<linux/kdev_t.h>
#include<linux/module.h>
#include<linux/moduleparam.h>
#include<linux/sched.h>
#include<linux/semaphore.h>
#include<linux/errno.h>
#include<linux/types.h>
#include<linux/uaccess.h>
#include<linux/wait.h>
#include<linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>	
#include <linux/pid.h>		/* For pid types */
#include <linux/tty.h>		/* For the tty declarations */
#include <linux/version.h>	/* For LINUX_VERSION_CODE */
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/kern_levels.h>


#define DRIVER_NAME "multiflow-driver"
#define DEVICE_NAME "mfdev"  /* Device file name in /dev/ - not mandatory  */
#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s:"fmt,DRIVER_NAME, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s:"fmt,DRIVER_NAME,##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s:"fmt,DRIVER_NAME, ##args)

static int Major;            /* Major number assigned to broadcast device driver */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session)	MAJOR(session->f_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_inode->i_rdev)
#else
#define get_major(session)	MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_dentry->d_inode->i_rdev)
#endif

//deferred work struct
struct __deferred_work_item {
        struct file *filp;
        char * bff;
        size_t len; 
        long long int off;   
        struct work_struct	w;
};
typedef struct __deferred_work_item deferred_work_t;

//session data struct 
struct __session_data {
  int prio;    //prio : 0 high prio - 1 low prio 
  int op;      //type of operation :  0 non blocking operation - 1 blocking operation
  int TIMEOUT; //timeout in millisecond
  unsigned long jiffies; //timeout in jiffies (HZ)
};
typedef struct __session_data session_data_t;

//device struct
typedef struct _device{
  struct mutex mutex_hi;
  struct mutex mutex_low;
  wait_queue_head_t hi_queue; //wait event queue for high pio requests
  wait_queue_head_t low_queue;  //wait event queue for low prio requess
  int hi_valid_bytes;
  struct workqueue_struct *wq; //workqueue struct 
  int low_valid_bytes;
  char * hi_prio_stream; //the I/O node is a buffer in memory
  char * low_prio_stream; //the I/O node is a buffer in memory
} device;


#define MINORS 128
device devices[MINORS];


//param array
int devices_state[MINORS];  //initially : 0 (ALL ENABLED)
int high_bytes[MINORS];
int low_bytes[MINORS];
int high_waiting[MINORS];
int low_waiting[MINORS]; 


