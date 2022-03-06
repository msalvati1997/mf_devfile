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


void * the_thread_write_hi(void* path){

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
	ioctl(fd,IOCTL_SETTIMER,10); //SET TIMER
	printf("Writing on high priority stream...\n");
	write(fd,DATA_HI,SIZE_HI);
	return NULL;
}
void * the_thread_read_hi(void* path){

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
	ioctl(fd,IOCTL_SETTIMER,10); //SET TIMER
	char * buff = malloc(sizeof(char *)*4);
	read(fd,buff,sizeof(char *)*4);
	printf("%s read\n",buff);
	return NULL;
}

void * the_thread_write_low(void* path){

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
	ioctl(fd,IOCTL_SETTIMER,2); //SET TIMER
	printf("Writing on low priority stream...\n");
	write(fd,DATA_LOW,SIZE_LOW);
	
	return NULL;

}

void * the_thread_read_low(void* path){

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
	ioctl(fd,IOCTL_SETTIMER,2); //SET TIMER
	char * buff = malloc(sizeof(char *)*4);
	read(fd,buff,sizeof(char *)*4);
	printf("%s read\n",buff);

	return NULL;

}
int main(int argc, char** argv){

     int ret;
     int major;
     int minors;
     char *path;
	 int timer;
     pthread_t tid;

     if(argc<4){
	printf("useg: prog pathname major minors\n");
	return -1;
     }
     timer=0;
     path = argv[1];
     major = strtol(argv[2],NULL,10);
     minors = strtol(argv[3],NULL,10);
     printf("creating %d minors for device %s with major %d\n",minors,path,major);

     for(i=0;i<minors;i++){
	sprintf(buff,"mknod %s%d c %d %i\n",path,i,major,i);
	system(buff);
	sprintf(buff,"%s%d",path,i);
	pthread_create(&tid,NULL,the_thread_write_hi,strdup(buff));
	pthread_create(&tid,NULL,the_thread_write_hi,strdup(buff));
	pthread_create(&tid,NULL,the_thread_write_hi,strdup(buff));
	sleep(2);
	pthread_create(&tid,NULL,the_thread_read_hi,strdup(buff));
	sleep(2);
	pthread_create(&tid,NULL,the_thread_read_hi,strdup(buff));
 }
     pause();
     return 0;
}
