
#define TYPE 'h'

/*
Macro's for IOTCL's commands: 

IOCTL_RESET 	    Allows to set the default setting parameters of the device file.
IOCTL_HIGH_PRIO 	Allows to set the workflow to high priority.
IOCTL_LOW_PRIO 	    Allows to set the workflow to low priority.
IOCTL_BLOCKING 	    Allows to set the working mode operation to non blocking.
IOCTL_NO_BLOCKING 	Allows to set the working mode operation to blocking.
IOCTL_SETTIMER    	Allows to set the timer for blocking operation.
IOCTL_ENABLE 	    Allows to set the device state to enable.
IOCTL_DISABLE    	Allows to set the device state to disable.
*/

#define IOCTL_RESET	_IO(TYPE, 0)

#define IOCTL_HIGH_PRIO _IO(TYPE,1)    
#define IOCTL_LOW_PRIO _IO(TYPE,2)    

#define IOCTL_BLOCKING _IO(TYPE,3)  
#define IOCTL_NO_BLOCKING _IO(TYPE,4)  

#define IOCTL_SETTIMER _IOWR(TYPE,5,int)  //milliseconds 

#define IOCTL_ENABLE _IO(TYPE,6)  
#define IOCTL_DISABLE _IO(TYPE,7)  

#define IOCTL_TIMER_TEST _IO(TYPE,8)  


#define DEFAULT_PRIORITY 1 // low prio
#define DEFAULT_OP 1 //  blocking
#define DEFAULT_TIMEOUT 2500 // milliseconds
#define DEFAULT_STATE 0 // default state - ENABLE
#define MAX_TIMEOUT 180000 // MAX TIMEOUT - 3min 

