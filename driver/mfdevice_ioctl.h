
#define TYPE 'h'


#define hi_ioctl _IOWR(TYPE,0,int,int)  // (type, commands, blocking/non blocking operations , TIMEOUT)
		

#define low_ioctl _IOWR(TYPE,1,int,int)  // (type, commands, blocking/non blocking operations , TIMEOUT)




