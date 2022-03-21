
Multi-flow device file 
==================
## Martina Salvati
Advanced Operating Systems - MS Degree in Computer Engineering - Academic Year 2021/202

## Project's description

This project is related to a Linux device driver implementing low and high priority flows of data. Through an open session to the device file a thread can read/write data segments. The data delivery follows a First-in-First-out policy along each of the two different data flows (low and high priority). After read operations, the read data disappear from the flow. Also, the high priority data flow must offer synchronous write operations while the low priority data flow must offer an asynchronous execution (based on delayed work) of write operations, while still keeping the interface able to synchronously notify the outcome. Read operations are all executed synchronously. The device driver should support 128 devices corresponding to the same amount of minor numbers.

## Installation
1. Clone this repo
    ```bash
   cd
   git clone https://github.com/msalvati1997/mf_devfile.git
   ```
2. Begin install
- The install script can be use in order to install/uninstall the module.

   ```bash
   cd ~/driver
   sudo ./install.sh
   ```
   ![alt text](https://github.com/msalvati1997/mf_devfile/main/driver/INSTALLATION_SCRIPT.png "WELCOME DISPLAY")
## Multiflow driver 

| Driver file operations - fops  |  
|---|
|  static int dev_open(struct inode *, struct file *);
  | static int dev_release(struct inode *, struct file *);
  |  static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
 |  static long dev_ioctl(struct file *filp, unsigned int command, unsigned long arg);
 |   |

## IOCTL - device file settings
The device parameters of files can be manipulated by the ioctl() system call. 
Some macros have been created that make it easier to set parameters (driver/multiflow-driver_ioctl.h)
|  MACRO | DESCRIPTION   |   |   |   |
|---|---|---|---|---|
|  IOCTL_RESET |   Allows  to set the default setting parameters of the device file.|   |   |   |
| IOCTL_HIGH_PRIO  |  Allows to set the workflow to high priority.  |   |   |   |
|   IOCTL_LOW_PRIO|    Allows to set the workflow to low priority.|   |   |   |
|  IOCTL_BLOCKING |  Allows to set the working mode operation to non blocking.  |   |   |   |
| IOCTL_NO_BLOCKING  |   Allows to set the working mode operation to  blocking.|   |   |   |
|   IOCTL_SETTIMER|  Allows to set the timer for blocking operation.  |   |   |   |
|  IOCTL_ENABLE |  Allows to set the device state to enable. |   |   |   |
| IOCTL_DISABLE  | Allows to set the device state to disable.  |   |   |   |


## A simple program-flow 

1. Open device :
    ```bash
	fd = open(device,O_RDWR|O_APPEND);
    ```
2. When open a device of multiflow-driver the files operation called is 
    ```
     static int dev_open(struct inode *, struct file *);
    ```
    - The operation creates structured session data with default settings. 
3. The IOCTL'S MACRO can be used in order to set the device file parameters. 
Example: 
   ```
    ioctl(fd, IOCTL_RESET); 
    ```	
4. After setting the parameters, a write or read operation can be called.
Example: 
    ```
            ##Example of write operation
     	    char * buff = malloc(sizeof(char)*7);
            write(fd,buff,strlen(buff));

            ##Example of read operation
            char * buff = malloc(sizeof(char)*7);
        	read(fd,buff,7);
    ```	

## Testing 