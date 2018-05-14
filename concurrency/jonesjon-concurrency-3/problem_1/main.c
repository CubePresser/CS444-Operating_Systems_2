#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> //For processes
#include <pthread.h> //For threads
#include <semaphore.h>
#include "mt19937ar.h"

//Constructed from equivalent python implementation in little book of semaphores page 70
typedef struct Lightswitch { 
	int counter;
	sem_t* mutex;
}Lightswitch;

void ls_lock(Lightswitch* ls, sem_t* s)
{
    //printf("Ya boi\n");
	sem_wait(ls->mutex);
	ls->counter += 1;
	if(ls->counter == 1)
		sem_wait(s);
	sem_post(ls->mutex);
}

void ls_unlock(Lightswitch* ls, sem_t* s)
{
	sem_wait(ls->mutex);
	ls->counter -= 1;
    //printf("Hello there\n");
	if(ls->counter == 0)
    {
        printf("Empty counter\n");
		sem_post(s);
    }
    //printf("Goodbye\n");
	sem_post(ls->mutex);
}
//////////////////////////////////////////////////////////////////////////////////////

typedef struct Args_t {
	Lightswitch* lock;
	sem_t* count;
    sem_t* empty;
    sem_t* talk;
}Args_t;

void driver();
unsigned int prng();
void* thread_f(void*);
pthread_t* get_threads(int num_threads, void* function, void* args);

int bit;

int main()
{
	unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;

    eax = 0x01;

    //Gets details about the processor chip (So we can check which psuedo random number generator it supports)
    //Communicates directly with the operating system for information
    //The "=a", "=b", "=c", "=d" tells the system to output the eax, ebx, ecx and edx register values after computation into the c containers we gave it
    __asm__ __volatile__(
        "cpuid;"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) //Send the outputs of the registers to our c values
        : "a"(eax) //Eax value will be the input (The 0x01 probably specifies that we want whatever information cpuid gives us from an argument of 1)
    );

    if (ecx & 0x40000000) 
    {
        printf("Using rdrand\n");
        bit = 1; //Use rdrand for INTEL processor chips
    }
    else {
        init_genrand(time(NULL)); //Seed the mt19937 random value generator with time(NULL)
        bit = 0; //Use mt19937 for the ENGR server and other processor chips
    }

    //Run main program code
    driver();

    return 0;
}

void driver()
{
    Lightswitch lock;
    lock.counter = 0;
    lock.mutex = (sem_t*)malloc(sizeof(sem_t));

	sem_t *count, *empty, *talk;
	count = (sem_t*)malloc(sizeof(sem_t));
    empty = (sem_t*)malloc(sizeof(sem_t));
    talk = (sem_t*)malloc(sizeof(sem_t));

    sem_init(lock.mutex, 0, 1);
    sem_init(empty, 0, 1);
	sem_init(count, 0, 3);
    sem_init(talk, 0, 1);

    Args_t arguments;
    arguments.lock = &lock;
    arguments.count = count;
    arguments.empty = empty;
    arguments.talk = talk;

	pthread_t *threads;
	threads = get_threads(3, thread_f, &arguments);

    pthread_join(threads[0], NULL);
	return;
}

void* thread_f(void* args)
{
	Args_t* arguments = (Args_t*)args;
	while(1)
	{
		ls_lock(arguments->lock, arguments->empty);
        sem_wait(arguments->count);
        sem_wait(arguments->talk);
		printf("%d: A thread is using the resource\n", arguments->lock->counter);
        sem_post(arguments->talk);
        //sleep(prng()%10+1);
        sleep(2);
        sem_wait(arguments->talk);
        printf(": A thread is finished using the resource\n");
        sem_post(arguments->talk);
        sem_post(arguments->count);
		ls_unlock(arguments->lock, arguments->empty);
        sleep(prng()%10+1);
	}
}

pthread_t* get_threads(int num_threads, void* function, void* args)
{
	pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*num_threads);
	int i; for(i = 0; i < num_threads; i++)
	{
		pthread_create(&(threads[i]), NULL, function, args);
	}
	return threads;
}

/*************************************************
 * Function: prng
 * Description: Psuedo Random Number Genrator. INTEL CHIP: Uses the rdrand asm instruction to generate a random number. Loops until the instruction has successfully
 * returned a random number. OTHER CHIPS: Uses the mt19937ar.h file functions to generate a random number.
 * Params: None
 * Returns: Random unsigned int
 * Pre-conditions: Processor chip has been identified correctly and bit is set to either 0 or 1 respectively.
 * Post-conditions: None
 * **********************************************/
unsigned int prng()
{
    unsigned int rnd = 0;
    unsigned char ok = 0; //Used to check if rdrand failed or not

    //If bit is 0 then use mt19937 else use rdrand
    if(bit)
    {
        while(!((int)ok))
        {
            __asm__ __volatile__ (
                "rdrand %0; setc %1"
                : "=r" (rnd), "=qm" (ok) //Get a random number using rdrand
            );
        }
    }
    else
        rnd = (unsigned int)genrand_int32(); //Get a random number using mt19937

    return rnd;
}