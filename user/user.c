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
	ioctl(fd,IOCTL_SETTIMER,1500); //SET TIMER in milliseconds
	char * buff = malloc(sizeof(char)*4);
	read(fd,buff,sizeof(char)*4);
	printf("READER FROM HIGH READ %s \n",buff);
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
	ioctl(fd,IOCTL_BLOCKING); //-blocking operations 
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
	ioctl(fd,IOCTL_BLOCKING); // blocking operations 
	ioctl(fd,IOCTL_SETTIMER,1500); //SET TIMER in milliseconds
	char * buff = malloc(sizeof(char)*4);
	read(fd,buff,sizeof(char)*4);
	printf("READED FROM LOW READ %s\n",buff);

	return NULL;

}
int main(int argc, char** argv)
{
	int i;
int action;
int minors;
unsigned long timeout;
char **minors_list;
pthread_t tid1, tid2, tid3, tid4;
char buff[4096];

	int major = strtol(argv[2],NULL,10);
    minors = strtol(argv[3],NULL,10);
    char *path = argv[1];
	minors_list = malloc(minors*sizeof(char*));

    if (argc < 4)
    {
        printf("ERROR - WRONG PARAMETERS: usage -> prog pathname major minors\n");
        return -1;
    }
    printf("\n----------Multi-flow device driver tester initialization started correctly.\n\n");
    printf("\t...Creating %d minors for device %s (major %d)\n", minors, path, major);
    for (i = 0; i < minors; i++)
    {
        sprintf(buff, "mknod %s%d c %d %i\n", path, i, major, i);
        system(buff);
        sprintf(buff, "%s%d", path, i);
        minors_list[i] = malloc(32);
        strcpy(minors_list[i], buff);
    }
    printf("\tSystem initialized. Minors list:\n");
    for (i = 0; i < minors; i++)
    {
        printf("\t\t%s\n", minors_list[i]);
    }
	printf("\n\nThis is a testing program. Starting tests...\n");
	printf("\n\tTest 1 - concurrent writes...\n");
	for(i=0;i<minors;i++)
	{
		pthread_create(&tid1, NULL, the_thread_write_low, strdup(minors_list[i]));
		pthread_create(&tid2, NULL, the_thread_write_low, strdup(minors_list[i]));
		pthread_create(&tid3, NULL, the_thread_write_low, strdup(minors_list[i]));
		pthread_create(&tid4, NULL, the_thread_write_low, strdup(minors_list[i]));
		pthread_join(tid1,NULL);
		pthread_join(tid2,NULL);
		pthread_join(tid3,NULL);
		pthread_join(tid4,NULL);
	
		sleep(1);

	}
	printf("\t\tdone.\n");


    return 0;
}