#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include "mfdevice_ioctl.h"
int i;
char buff[4096];
#define DATA_HI "HIGH PRIO  "
#define DATA_LOW "LOW PRIO   "

#define SIZE_HI strlen(DATA_HI)
#define SIZE_LOW strlen(DATA_LOW)


void * the_thread_hi(void* path, int timer){

	char* device;
	int fd;

	device = (char*)path;
	sleep(1);

	printf("opening device %s\n",device);
	fd = open(device,O_RDWR|O_APPEND);
	if(fd == -1) {
		printf("open error on device %s\n",device);
		return NULL;
	}
	printf("device %s successfully opened\n",device);
    /*int err = ioctl(fd, IOCTL_RESET);	//reset*/
	ioctl(fd, IOCTL_HIGH_PRIO); //high
	ioctl(fd,IOCTL_BLOCKING); //blocking operations 	
	ioctl(fd,IOCTL_SETTIMER,timer); //SET TIMER
	printf("Writing on high priority stream...\n");
	write(fd,DATA_HI,SIZE_HI);
	return NULL;
}

void * the_thread_low(void* path, int timer){

	char* device;
	int fd;

	device = (char*)path;
	sleep(1);

	printf("opening device %s\n",device);
	fd = open(device,O_RDWR|O_APPEND);
	if(fd == -1) {
		printf("open error on device %s\n",device);
		return NULL;
	}
	printf("device %s successfully opened\n",device);
	/*int err = ioctl(fd, IOCTL_RESET);	//reset*/
	ioctl(fd, IOCTL_LOW_PRIO); //low priority  
	ioctl(fd,IOCTL_NO_BLOCKING); //- no blocking operations 
	ioctl(fd,IOCTL_SETTIMER,timer); //SET TIMER
	printf("Writing on low priority stream...\n");
	write(fd,DATA_LOW,SIZE_LOW);
	
	return NULL;

}
int main(int argc, char** argv){

     int ret;
     int major;
     int minors;
     char *path;
     pthread_t tid;

     if(argc<4){
	printf("useg: prog pathname major minors\n");
	return -1;
     }

     path = argv[1];
     major = strtol(argv[2],NULL,10);
     minors = strtol(argv[3],NULL,10);
     printf("creating %d minors for device %s with major %d\n",minors,path,major);

     for(i=0;i<minors;i++){
	sprintf(buff,"mknod %s%d c %d %i\n",path,i,major,i);
	system(buff);
	sprintf(buff,"%s%d",path,i);
	pthread_create(&tid,NULL,the_thread_hi,strdup(buff),2);
	pthread_create(&tid,NULL,the_thread_hi,strdup(buff),5);
	pthread_create(&tid,NULL,the_thread_hi,strdup(buff),1);
	pthread_create(&tid,NULL,the_thread_hi,strdup(buff),6);
     }

     pause();
     return 0;

}
