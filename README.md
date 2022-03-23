  

Multi-flow device file
==================

## Martina Salvati
Advanced Operating Systems - MS Degree in Computer Engineering - Academic Year 2021/2022


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
## Project's file
  
  The directory ./driver contains all files directly used in the driver's logic : 

  -  ./multiflow-driver.c : implementation of the logic of the driver operations
  -  ./multiflow-driver.h : specification of the data structures used in the project 
  - ./multiflow-driver_ioctl.h : contains the macros used for ioctl() operation 
  - ./install.sh : the install script of the modules
  
  The directory ./test contains all files directly used in the test's logic : 
  
  - ./test.h : contains all the functions and data structure used for test
  - ./simple_test.c : test of write and read in different session
  - ./write_and_read_test.c : test of write and read in same session
  - ./concurrency_test.c : test of concurrent writes and reads
  - ./param_test.c : test of device params
  - ./timeout_test.c : test of timeout's expiration
  - ./enable_disable_test.c test of enable/disable param 
  - ./test.sh is the script used to call the test function


Module details
==================

In open phase, the module initializes all the data structures needed by the device driver. For each minor (device file) initializes the high and low priority streams with the relative workqueue and waitqueue.

During the module exit phase,  the driver's structures are deallocated.


Module's parameters :

|  PARAM NAME 	|  TYPE  	|  DESCRIPTION  	|   	
|---	|---	|---	|
|   	devices_state |  array of int	| This param set the state of the device file. If set to enable, new session can be created. If set to disable, new session can't be created.    	| 
|   low_waiting	|  array of int 	|   This param indicates the thread currently waiting on the waitqueue reserved to the process working to the low stream.	|   	
|   hi_waiting	|   array of int	|    This param indicates the thread currently waiting on the waitqueue reserved to the process working to the hi stream.	|   	
|   high_bytes	|   array of int	|    This param indicates the number of bytes present on the high prio stream	|  
|   low_bytes	|   array of int	|    This param indicates the number of bytes present on the low prio stream|  

- To see the module's parameters for device driver : 
  ```bash
  sudo cat /sys/module/multiflow_driver/parameters/#PARAM NAME
   ```
- To modify the ENABLE/DISABLE parameter for device driver : 
   ```bash
  sudo echo 0,0,1,.. /sys/module/multiflow_driver/parameters/device_state
   ```
   The enable and disable parameter can be easily changed via the IOCTL! 
   
   ```bash
     #ENABLE DEVICE 
     ioctl(fd, IOCTL_ENABLE);

     #DISABLE DEVICE
     ioctl(fd,IOCTL_DISABLE);

   ```




Driver details
==================


## Data structure 
-----------------------------
The multiflow-driver.h contains all the data structure of the device driver. 

<p>The structure reserved for each device :</p>
<pre><code>typedef struct _device{
  struct mutex mutex_hi;  //synchronizer of high prio stream
  struct mutex mutex_low; //synchronizer of low prio stream
  wait_queue_head_t hi_queue; //wait event queue for high pio requests
  wait_queue_head_t low_queue;  //wait event queue for low prio requess
  int hi_valid_bytes;
  struct workqueue_struct *wq; //workqueue struct 
  int low_valid_bytes;
  char * hi_prio_stream; //the I/O node is a buffer in memory
  char * low_prio_stream; //the I/O node is a buffer in memory
} device;
</code></pre>

When a program opens a session, it'a necessary to allocate a private structure that could keep the parameters set by the user:

-  PRIO : the priority of the working stream  (high prio/low prio)
- OP : the type of operation (blocking/non blocking)
- TIMEOUT : the timeout for the blocking operation 
- JIFFIES : the timeout expressed in jiffies (HZ)


<p>The structure reserved for each session opened by a program:</p>
<pre><code>struct __session_data {
  int prio;    //prio : 0 high prio - 1 low prio 
  int op;      //type of operation :  0 non blocking operation - 1 blocking operation
  int TIMEOUT; //timeout in millisecond
  unsigned long jiffies; //timeout in jiffies (HZ)
};
typedef struct __session_data session_data_t;
</code></pre>

<p> For each workqueue it's necessary to implement a structure that could contain the data necessary for the write operations of the low priority stream.
The structure reserved for each item of the worqueue :</p>
<pre><code>struct __deferred_work_item {
        struct file *filp;
        char * bff;
        size_t len; 
        long long int off;   
        struct work_struct	w;
};
typedef struct __deferred_work_item deferred_work_t;
</code></pre>


## Waitqueue
-----------------------------

To implementate the synchronous blocking work it was necessary to use waitqueues.
There are two different queues for each stream (low/high priority).
The API was used to manage the queues:

   ```bash
      wait_event_timeout(wq, condition, timeout); 	 
   ```

The process is put to sleep until the lock is taken :

   ```bash
      int mutex_trylock (struct mutex * lock); 	
   ```

When the timeouot expired the process exit from the execution flow. 
If the condition evaluates to true before the expiration of the timeout the process continues its flow of execution.


## Workqueue : implementation of delayed work
-----------------------------
To implementate the async deferred work it was necessary to work with workqueues,
For each device is allocated one workqueue :
```bash
alloc_workqueue(fmt, flags, max_active, args...); 
 ```
It was decided to use this type of API because it allows you to manage a good level of concurrency.
The following function it's used to enqueue the __deferred_work_item (filled with op's vars) in the workqueue :
```bash
bool queue_work(struct workqueue_struct * wq, struct work_struct * work);
 ```

## IOCTL - device file settings
-----------------------------
The device parameters of files can be manipulated by the ioctl() system call.

Some macros have been created that make it easier to set parameters (driver/multiflow-driver_ioctl.h)

| MACRO | DESCRIPTION |
|---|---|
| IOCTL_RESET | Allows to set the default setting parameters of the device file.| | | |
| IOCTL_HIGH_PRIO | Allows to set the workflow to high priority. | 
| IOCTL_LOW_PRIO| Allows to set the workflow to low priority.| 
| IOCTL_BLOCKING | Allows to set the working mode operation to non blocking. | 
| IOCTL_NO_BLOCKING | Allows to set the working mode operation to blocking.| 
| IOCTL_SETTIMER| Allows to set the timer for blocking operation. | 
| IOCTL_ENABLE | Allows to set the device state to enable. | 
| IOCTL_DISABLE | Allows to set the device state to disable. | 

FOPS
=======================

## Device driver table
-----------------------------
| Driver file operations - fops |
|---|
| static int dev_open(struct inode *, struct file *);
| static int dev_release(struct inode *, struct file *);
| static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
| static long dev_ioctl(struct file *filp, unsigned int command, unsigned long arg);

Open
 -----------------------------
The device opening operation allocates a private session structure. The allocation of the structure is allowed only if the device_state of the object is set to ENABLE, otherwise (DISABLE) it is not possible to create new sessions. 

Writes
 -----------------------------
The write operation of n bytes involves the allocation of n bytes of memory. 
Memory is dynamically allocated through the functions:

```bash
void * krealloc(const void * p, size_t new_size, gfp_t flags); //used to add new bytes
 ```

```bash
void *memset(void *s, int c, size_t n);//used to initialize the memory allocated
 ```

To copy data from user space to kernel space : 
```bash
unsigned long  copy_from_user(void * to, const void __user * from, unsigned long n);
 ```
The write operation has a different behavior for each session's setting parameters :

* **HIGH PRIO STREAM** :
    * **BLOCKING OPERATION**
      The blocking operations work with the waitqueues. The process waits as long as the condition is true and the timeout has not expired. The condition is about taking the lock (reserved of the high level stream) in order to do the operation. 
    * **NON BLOCKING OPERATION** : The non blocking operation work with the mutex_trylock API. It is not blocking: if the resource is busy, control returns to the user.

    
* **LOW PRIO STREAM** : A **workqueue deferred item** is allocated. Then the work is enqueue to the workqueue. 
     * **BLOCKING OPERATION**
       The blocking operations work with the waitqueues. The process waits as long as the condition is met and the timeout has not expired. The condition is about taking the lock (reserved of the high level stream) in order to do the operation. 
    * **NON BLOCKING OPERATION** : The non blocking operation work with the mutex_trylock API. It is not blocking: if the resource is busy, control returns to the user.

  

Reads
 -----------------------------
  Readings are performed in FIFO mode from left to right. When a read is performed then the readed bytes are removed from the stream.

To copy data from kernel space to user space:  
  ```bash
unsigned long copy_to_user(void __user * to, const void * from, unsigned long n); 
 ```

To manage memory deallocation after read : 

```bash
void *memmove(void *dest, const void *src, size_t n); // used to copyes n-read bytes 
 ```

 ```bash
void *memset(void *s, int c, size_t n);//clear the last bytes
 ```

  ```bash
void * krealloc(const void * p, size_t new_size, gfp_t flags); //used to remove readed bytes
 ```
Read operation depends on the priority of the working stream: 

  * **HIGH PRIO STREAM** :
    * **BLOCKING OPERATION**
      The blocking operations work with the waitqueues. The process waits as long as the condition is met and the timeout has not expired. The condition is about taking the lock (reserved of the high level stream) in order to do the operation. 
    * **NON BLOCKING OPERATION** : The non blocking operation work with the mutex_trylock API. It is not blocking: if the resource is busy, control returns to the user.

    
* **LOW PRIO STREAM** : 
     * **BLOCKING OPERATION**
       The blocking operations work with the waitqueues. The process waits as long as the condition is met and the timeout has not expired. The condition is about taking the lock (reserved of the high level stream) in order to do the operation. 
    * **NON BLOCKING OPERATION** : The non blocking operation work with the mutex_trylock API. It is not blocking: if the resource is busy, control returns to the user.

In this case the workqueue are not used : the read operation is always synchronized. 


Release 
 -----------------------------
This operation is used to close a specific device and to deallocate the session's private data. 

Usage
 -----------------------------


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


