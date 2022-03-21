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

#define IOCTL_ENABLE _IO(TYPE,6)  
#define IOCTL_DISABLE _IO(TYPE,7)  


#define DEFAULT_PRIO 1 // low prio
#define DEFAULT_OP 0 // non blocking
#define DEFAULT_TIMEOUT 5000 // milliseconds
#define DEFAULT_STATE 0 // default state - ENABLE

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
	char * buff = malloc(sizeof(char)*8);
	char* data = rand_string_alloc(sizeof(char)*7);
	buff = strcat(data," ");
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
	ioctl(fd,IOCTL_SETTIMER,3500); //SET TIMER in milliseconds


    printf("Writing on high priority stream... : %s \n",buff);
	write(fd,buff,strlen(buff));
	close(fd);

	return NULL;
}
void * the_thread_read_hi_block(void* path){

	char* device;
	int fd;
    char * buff = malloc(sizeof(char)*7);
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
	ioctl(fd,IOCTL_SETTIMER,3000); //SET TIMER in milliseconds
	
	read(fd,buff,7);
	printf("Readed from high level stream %s \n",buff);
	close(fd);
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
	char* data = rand_string_alloc(sizeof(char)*7);
	buff = strcat(data," ");
	printf("Writing on low priority stream...: %s \n",buff);
	write(fd,buff,strlen(buff));

	close(fd);
	
	return NULL;

}

void * the_thread_write_low_disable(void* path){

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
	char* data = rand_string_alloc(sizeof(char)*7);
	buff = strcat(data," ");
	printf("Writing on low priority stream...: %s \n",buff);
	write(fd,buff,strlen(buff));

	ioctl(fd,IOCTL_DISABLE);

	write(fd,buff,strlen(buff));
	

	ioctl(fd,IOCTL_ENABLE);
	close(fd);

	return NULL;

}

void * the_thread_read_low_block(void* path){

	char* device;
	int fd;
	char * buff = malloc(sizeof(char)*7);

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
	ioctl(fd,IOCTL_SETTIMER,3000); //SET TIMER in milliseconds
	read(fd,buff,7);
	printf("Readed from low level stream %s \n",buff);
	close(fd);

	return NULL;

}

void * the_thread_write_hi_nb(void* path){

	char* device;
	int fd;
    char * buff = malloc(sizeof(char)*8);
	char* data = rand_string_alloc(sizeof(char)*7);
	buff = strcat(data," ");
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
	ioctl(fd,IOCTL_SETTIMER,3500); //SET TIMER in milliseconds
	
    printf("Writing on high priority stream...%s \n",buff);
	write(fd,buff,strlen(buff));
	close(fd);

	return NULL;
}
void * the_thread_read_hi_nb(void* path){

	char* device;
	int fd;
	char * buff = malloc(sizeof(char)*7);

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
	ioctl(fd,IOCTL_SETTIMER,3000); //SET TIMER in milliseconds
	read(fd,buff,sizeof(char)*7);
	printf("Readed from high level stream %s \n",buff);
	close(fd);

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
	ioctl(fd,IOCTL_SETTIMER,5000); //SET TIMER in milliseconds
	char * buff = malloc(sizeof(char)*8);
	char* data = rand_string_alloc(sizeof(char)*7);
	buff = strcat(data," ");
	printf("Writing on low priority stream...%s \n",buff);
	write(fd,buff,strlen(buff));
	
	close(fd);
	
	return NULL;

}

void * the_thread_read_low_nb(void* path){

	char* device;
	int fd;
	char * buff = malloc(sizeof(char)*7);

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
	ioctl(fd,IOCTL_SETTIMER,3000); //SET TIMER in milliseconds
	read(fd,buff,7);
	printf("Readed from low level stream %s \n",buff);
	close(fd);

	return NULL;

}

void * the_thread_write_and_read_low(void* path){

	char* device;
	int fd;

	device = (char*)path;
	char * buff_write = NULL;
	buff_write= malloc(sizeof(char)*8);
	char * buff_read =  NULL;
	buff_read= malloc(sizeof(char)*8);
	char* data = rand_string_alloc(sizeof(char)*7);
	buff_write = strcat(data," ");
	sleep(3);

	printf("opening device %s\n",device);
	fd = open(device,O_RDWR);
	if(fd == -1) {
		printf("open error on device %s\n",device);
		return NULL;
	}
	printf("device %s successfully opened\n",device);
	/*int err = ioctl(fd, IOCTL_RESET);	//reset*/
	ioctl(fd, IOCTL_LOW_PRIO); //low priority  
	ioctl(fd,IOCTL_NO_BLOCKING); // no blocking operations 
	ioctl(fd,IOCTL_SETTIMER,3000); //SET TIMER in milliseconds
	sleep(1);
	

	printf("Writing to low level stream %s\n",buff_write);	
	write(fd,buff_write,strlen(buff_write));

	sleep(5);
	printf("Trying to read my 8 bytes \n");
	read(fd,buff_read,7);
	
	printf("Readed from low level stream %s \n",buff_read);

	sleep(1);
	close(fd);
	sleep(1);

	return NULL;

}

void * the_thread_write_and_read_hi(void* path){

	char* device;
	int fd;
	char * buff_write = NULL;
	char * buff_read =  NULL;
	buff_write= malloc(sizeof(char)*8);
	buff_read= malloc(sizeof(char)*8);
	char* data = rand_string_alloc(sizeof(char)*7);
	buff_write = strcat(data," ");

	device = (char*)path;
	sleep(3);

	printf("opening device %s\n",device);
	fd = open(device,O_RDWR);
	if(fd == -1) {
		printf("open error on device %s\n",device);
		return NULL;
	}
	printf("device %s successfully opened\n",device);
	/*int err = ioctl(fd, IOCTL_RESET);	//reset*/
	ioctl(fd, IOCTL_HIGH_PRIO); //low priority  
	ioctl(fd,IOCTL_NO_BLOCKING); // no blocking operations 
	ioctl(fd,IOCTL_SETTIMER,6000); //SET TIMER in milliseconds
	sleep(1);
	
	
	printf("Writing to hi level stream %s\n",buff_write);	
	write(fd,buff_write,strlen(buff_write));

	sleep(5);
	printf("Trying to read my 8 bytes \n");
	read(fd,buff_read,7);
	
	printf("Readed from hi level stream %s \n",buff_read);
	sleep(1);

	close(fd);
	sleep(1);

	return NULL;

}

void * the_thread_read_all_low(void* path){

	char* device;
	int fd;
	char * buff = malloc(sizeof(char)*4096);

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
	ioctl(fd,IOCTL_SETTIMER,30000000000000); //SET TIMER in milliseconds
	read(fd,buff,4096);
	printf("Readed ALL low level stream %s \n",buff);
	close(fd);

	return NULL;

}

void * the_thread_read_all_hi(void* path){

	char* device;
	int fd;
	char * buff = malloc(sizeof(char)*4096);

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
	ioctl(fd, IOCTL_HIGH_PRIO); //low priority  
	ioctl(fd,IOCTL_BLOCKING); // blocking operations 
	ioctl(fd,IOCTL_SETTIMER,30000000000); //SET TIMER in milliseconds
	read(fd,buff,4096);
	printf("Readed ALL high level stream %s \n",buff);
	close(fd);

	return NULL;

}