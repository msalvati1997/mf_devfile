#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
int i;
char buff[4096];
#define DATA_HI "high"
#define DATA_LOW "low"
#define TYPE 'h'

#define SIZE_HI strlen(DATA_HI)
#define SIZE_LOW strlen(DATA_LOW)
#define IOCTL_RESET	_IO(TYPE, 0)

#define IOCTL_HIGH_PRIO _IO(TYPE,1)    
#define IOCTL_LOW_PRIO _IO(TYPE,2)    

#define IOCTL_BLOCKING _IO(TYPE,3)  
#define IOCTL_NO_BLOCKING _IO(TYPE,4)  

#define IOCTL_SETTIMER _IOWR(TYPE,5,int)  

static char *rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK...";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

char* rand_string_alloc(size_t size)
{
     char *s = malloc(size + 1);
     if (s) {
         rand_string(s, size);
     }
     return s;
}

void * the_thread_write_hi_block(void* path){

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
	ioctl(fd,IOCTL_SETTIMER,1000); //SET TIMER in milliseconds
	char * buff = malloc(sizeof(char)*8);
	char* data = rand_string_alloc(sizeof(char)*5);
	buff = strcat(data,"_");
	buff = strcat(data,DATA_HI);
	buff = strcat(data,"\n");
    printf("Writing on high priority stream... : %s \n",buff);
	write(fd,buff,strlen(buff));
	return NULL;
}
void * the_thread_read_hi_block(void* path){

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
	ioctl(fd,IOCTL_SETTIMER,1500); //SET TIMER in milliseconds
	char * buff = malloc(sizeof(char)*8);
	read(fd,buff,sizeof(char)*8);
	printf("Readed from high level stream %s \n",buff);
	return NULL;
}

void * the_thread_write_low_block(void* path){

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
	ioctl(fd,IOCTL_BLOCKING); //-blocking operations 
	ioctl(fd,IOCTL_SETTIMER,2500); //SET TIMER in milliseconds
	char * buff = malloc(sizeof(char)*8);
	char* data = rand_string_alloc(sizeof(char)*5);
	buff = strcat(data,"_");
	buff = strcat(data,DATA_LOW);
	buff = strcat(data,"\n");
	printf("Writing on low priority stream...: %s \n",buff);
	write(fd,buff,strlen(buff));
	
	return NULL;

}

void * the_thread_read_low_block(void* path){

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
	ioctl(fd,IOCTL_BLOCKING); // blocking operations 
	ioctl(fd,IOCTL_SETTIMER,1500); //SET TIMER in milliseconds
	char * buff = malloc(sizeof(char)*8);
	read(fd,buff,sizeof(char)*8);
	printf("Readed from low level stream %s \n",buff);

	return NULL;

}

void * the_thread_write_hi_nb(void* path){

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
	ioctl(fd,IOCTL_NO_BLOCKING); // no blocking operations 
	ioctl(fd,IOCTL_SETTIMER,1000); //SET TIMER in milliseconds
	char * buff = malloc(sizeof(char)*8);
	char* data = rand_string_alloc(sizeof(char)*5);
	buff = strcat(data,"_");
	buff = strcat(data,DATA_HI);
	buff = strcat(data,"\n");
    printf("Writing on high priority stream...%s \n",buff);
	write(fd,buff,strlen(buff));
	return NULL;
}
void * the_thread_read_hi_nb(void* path){

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
	ioctl(fd,IOCTL_NO_BLOCKING); // no blocking operations 
	ioctl(fd,IOCTL_SETTIMER,1500); //SET TIMER in milliseconds
	char * buff = malloc(sizeof(char)*8);
	read(fd,buff,sizeof(char)*8);
	printf("Readed from high level stream %s \n",buff);
	return NULL;
}

void * the_thread_write_low_nb(void* path){

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
	ioctl(fd,IOCTL_NO_BLOCKING); // no blocking operations 
	ioctl(fd,IOCTL_SETTIMER,2500); //SET TIMER in milliseconds
	char * buff = malloc(sizeof(char)*8);
	char* data = rand_string_alloc(sizeof(char)*5);
	buff = strcat(data,"_");
	buff = strcat(data,DATA_LOW);
	buff = strcat(data,"\n");
	printf("Writing on low priority stream...%s \n",buff);
	write(fd,buff,strlen(buff));
	
	return NULL;

}

void * the_thread_read_low_nb(void* path){

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
	ioctl(fd,IOCTL_NO_BLOCKING); // no blocking operations 
	ioctl(fd,IOCTL_SETTIMER,1500); //SET TIMER in milliseconds
	char * buff = malloc(sizeof(char)*8);
	read(fd,buff,sizeof(char)*8);
	printf("Readed from low level stream %s \n",buff);

	return NULL;

}