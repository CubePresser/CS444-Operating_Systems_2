#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> //For processes
#include <pthread.h> //For threads
#include <sys/mman.h>

int main()
{
	printf("Program is running!\n");

	//Create a process
	pid_t process_id;

	pthread_mutex_t *myMutex;
	pthread_mutexattr_t attrmutex;

	pthread_mutexattr_init(&attrmutex);
	pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);

	myMutex = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
	//myMutex = malloc(sizeof(pthread_mutex_t));

	pthread_mutex_init(myMutex, &attrmutex);
	pthread_mutex_unlock(myMutex);

	process_id = fork();

	if(process_id == -1)
	{
		perror("Bad process\n");
		exit(1);
	}
	else if(process_id == 0)
	{
		while(1)
		{
			printf("Child ready\n");
			pthread_mutex_lock(myMutex);
			printf("CHILD: Using stdout for 2 seconds.\n");
			sleep(2);
			pthread_mutex_unlock(myMutex);
			printf("Child done\n");
			sleep(1);
		}
	}
	while(1)
	{
		printf("Parent ready\n");
		pthread_mutex_lock(myMutex);
		printf("PARENT: Using stdout for 2 seconds.\n");
		sleep(2);
		pthread_mutex_unlock(myMutex);
		printf("Parent done\n");
		sleep(1);
	}

	wait(&process_id);
	pthread_mutex_destroy(myMutex);
	pthread_mutexattr_destroy(&attrmutex);

	return 0;
}
