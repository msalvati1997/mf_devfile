#include "test.h"

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
    printf("\n----------Multi-flow device driver tester initialization started correctly.----------\n\n");
    printf("...Creating %d minors for device %s (major %d)\n", minors, path, major);
    for (i = 0; i < minors; i++)
    {
        sprintf(buff, "mknod %s%d c %d %i\n", path, i, major, i);
        system(buff);
        sprintf(buff, "%s%d", path, i);
        minors_list[i] = malloc(32);
        strcpy(minors_list[i], buff);
    }
    printf("System initialized. Minors list:\n");
    for (i = 0; i < minors; i++)
    {
        printf("\t%s\n", minors_list[i]);
    }
	printf("\n\nThis is a testing program. Starting tests...\n");
	printf("\nTest  -   write and read in same session in different flow..\n");
	for(i=0;i<minors;i++)
	{
		pthread_create(&tid1, NULL, the_thread_write_and_read_hi, strdup(minors_list[i]));
        sleep(1);
    	pthread_create(&tid2, NULL, the_thread_write_and_read_low, strdup(minors_list[i]));
		pthread_join(tid1,NULL);
        pthread_join(tid2,NULL);

	}
	printf("\n\tTest complete \n");

	printf("\t\tdone.\n");
    return 0;
}