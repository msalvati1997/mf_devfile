
#define TYPE 'h'

#define IOCTL_RESET	_IO(TYPE, 0)

#define IOCTL_HIGH_PRIO _IO(TYPE,1)    
#define IOCTL_LOW_PRIO _IO(TYPE,2)    

#define IOCTL_BLOCKING _IO(TYPE,3)  
#define IOCTL_NO_BLOCKING _IO(TYPE,4)  

#define IOCTL_SETTIMER _IOWR(TYPE,5,int)  //milliseconds 

#define IOCTL_ENABLE _IO(TYPE,6)  
#define IOCTL_DISABLE _IO(TYPE,7)  

#define IOCTL_TIMER_TEST _IO(TYPE,8)  


#define DEFAULT_PRIO 1 // low prio
#define DEFAULT_OP 1 //  blocking
#define DEFAULT_TIMEOUT 2500 // milliseconds
#define DEFAULT_STATE 0 // default state - ENABLE
#define MAX_TIMEOUT 180000 // MAX TIMEOUT - 3min 

