#include "test.h"
void * the_thread_timeout_expired_high(void* path){

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
	ioctl(fd,IOCTL_BLOCKING); // no blocking operations 
	ioctl(fd,IOCTL_SETTIMER,100); //SET TIMER in milliseconds  - in this case 1 [HZ]
    char * buff = malloc(sizeof(char)*8);
	char* data = rand_string_alloc(sizeof(char)*7);
	buff = strcat(data,"_");
    printf("Writing on high priority stream...%s \n",buff);
	write(fd,buff,strlen(buff));
    char * buff2 = malloc(sizeof(char)*8);

	read(fd,buff2,8);
	printf("Read from high priority stream.. %s\n", buff2);
	return NULL;
}

void * the_thread_timeout_expired_low(void* path){

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
	ioctl(fd, IOCTL_LOW_PRIO); //high
	ioctl(fd,IOCTL_BLOCKING); // no blocking operations 
	ioctl(fd,IOCTL_SETTIMER,100); //SET TIMER in milliseconds
    char * buff = malloc(sizeof(char)*8);
	char* data = rand_string_alloc(sizeof(char)*7);
	buff = strcat(data,"_");
    printf("Writing on low priority stream...%s \n",buff);
	write(fd,buff,strlen(buff));
    char * buff2 = malloc(sizeof(char)*8);
	read(fd,buff2,8);
	printf("Read from low priority stream.. %s\n", buff2);
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
	printf("\n\tTest - timeout expired..\n");

	for(i=0;i<minors;i++)
	{
		pthread_create(&tid1, NULL, the_thread_timeout_expired_high, strdup(minors_list[i]));
		pthread_create(&tid2, NULL, the_thread_timeout_expired_low, strdup(minors_list[i]));
		pthread_create(&tid3, NULL, the_thread_timeout_expired_high, strdup(minors_list[i]));
		pthread_create(&tid4, NULL, the_thread_timeout_expired_low, strdup(minors_list[i]));
		pthread_join(tid1,NULL);
		pthread_join(tid2,NULL);
		pthread_join(tid3, NULL);
		pthread_join(tid4, NULL);
		sleep(1);

	}
	printf("\t\tdone.\n");

    return 0;
}

