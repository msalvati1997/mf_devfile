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
#define DATA_HI " ALTA PRIORITA \n"
#define DATA_LOW " BASSA PRIORITA \n"

#define SIZE_HI strlen(DATA_HI)
#define SIZE_LOW strlen(DATA_LOW)


void * the_thread_hi(void* path){

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
	ioctl(fd, hi_ioctl, 1); //high priority  - no blocking operations 
	printf("Writing on high priority stream...");
	write(fd,DATA_HI,SIZE_HI);
	return NULL;

}

void * the_thread_low(void* path){

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
	ioctl(fd, low_ioctl, 1); //low priority  - no blocking operations 
	printf("Writing on low priority stream...");
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
	pthread_create(&tid,NULL,the_thread_hi,strdup(buff));
	sleep(2);
	pthread_create(&tid,NULL,the_thread_hi,strdup(buff));

     }

     pause();
     return 0;

}
