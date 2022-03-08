
#define DRIVER_NAME "multiflow-driver"
#define DEVICE_NAME "mf-dev"  /* Device file name in /dev/ - not mandatory  */
#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s:"fmt,DRIVER_NAME, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s:"fmt,DRIVER_NAME,##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s:"fmt,DRIVER_NAME, ##args)
#include "multiflow-driver_ioctl.h"
#include<linux/capability.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/fs.h>
#include<linux/init.h>
#include<linux/ioctl.h>
#include<linux/kdev_t.h>
#include<linux/module.h>
#include<linux/moduleparam.h>
#include<linux/sched.h>
#include<linux/semaphore.h>
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
