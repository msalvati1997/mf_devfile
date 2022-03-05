
#define TYPE 'h'

#define IOCTL_RESET	_IO(TYPE, 0)

#define IOCTL_HIGH_PRIO _IO(TYPE,1)    
#define IOCTL_LOW_PRIO _IO(TYPE,2)    

#define IOCTL_BLOCKING _IO(TYPE,3)  
#define IOCTL_NO_BLOCKING _IO(TYPE,4)  

#define IOCTL_SETTIMER _IOWR(TYPE,5,int)  

#define DEFAULT_PRIO 1 // low prio
#define DEFAULT_OP 0 // non blocking
#define DEFAULT_TIMER 2 // 2 second