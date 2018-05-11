//////////////////////////////////////////////////////
// Jonathan Alexander Jones - jonesjon 932709446
// Concurrency 2
// CS444 Spring2018
//////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "mt19937ar.h"

#define NUM_PHILOSOPHERS 5

//Named philosophers for easy index access
enum philosophers {
    Aristotle,
    Socrates,
    Plato,
    Pythagoras,
    Democritus
};

//Holds the arguments to the thread function philosopher
typedef struct Args {
    int name;
    unsigned int* status;
    sem_t* talk;
    sem_t* footman;
    sem_t* forks[NUM_PHILOSOPHERS];
}Args;

//Function prototypes
void driver();
void* philosopher(void*);
void show_status(unsigned int*);
void show_name(int);
void get_forks(int, sem_t*, sem_t**);
void put_forks(int, sem_t*, sem_t**);
int right(int);
unsigned int prng();

//Global variables
int bit;

/* SOLUTION: From the little book of semaphores page 93
 *
 * def  get_forks(i):
 *     footman.wait()
 *     fork[right(i)]. wait()
 *     fork[left(i)]. wait()
 * def  put_forks(i):
 *    fork[right(i)]. signal ()
 *    fork[left(i)]. signal ()
 *    footman.signal ()
 */

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

/*************************************************
 * Function: driver
 * Description: Runs the solution for the dining philosophers problem. Sets up semaphores and threads for execution.
 * Params: None
 * Returns: None
 * Pre-conditions: Bit is set for random number generation.
 * Post-conditions: None
 * **********************************************/
void driver()
{
    //Initialize the values in the argument struct by allocating dynamic memory for pointers
    Args pt_args;
    pt_args.status = (unsigned int*)malloc(sizeof(unsigned int)*5);
    pt_args.talk = (sem_t*)malloc(sizeof(sem_t));
    pt_args.footman = (sem_t*)malloc(sizeof(sem_t));

    sem_init(pt_args.talk, 0, 1); //Semaphore for stdout control
    sem_init(pt_args.footman, 0, NUM_PHILOSOPHERS - 1); //Semaphore for number of o at table

    //Allocate and initialize the status array of ints and forks array of semaphores
    int i; for(i = 0; i < NUM_PHILOSOPHERS; i++)
    {
        pt_args.status[i] = 0;
        pt_args.forks[i] = (sem_t*)malloc(sizeof(sem_t));
        sem_init(pt_args.forks[i], 0, 1); //Semaphore for a single fork on the table
    }

    //Initialize pthread variables and generate arguments structs for each philosopher
    pthread_t aristotle, socrates, plato, pythagoras, democritus;
    Args aristotle_args = pt_args, socrates_args = pt_args, plato_args = pt_args, pythagoras_args = pt_args, democritus_args = pt_args;

    //Create the philosopher threads with their respective arguments and names
    aristotle_args.name = Aristotle;
    pthread_create(&aristotle, NULL, philosopher, &aristotle_args);

    socrates_args.name = Socrates;
    pthread_create(&socrates, NULL, philosopher, &socrates_args);

    plato_args.name = Plato;
    pthread_create(&plato, NULL, philosopher, &plato_args);

    pythagoras_args.name = Pythagoras;
    pthread_create(&pythagoras, NULL, philosopher, &pythagoras_args);

    democritus_args.name = Democritus;
    pthread_create(&democritus, NULL, philosopher, &democritus_args);

    //Block the parent thread until completion of the aristotle thread (Which never happens)
    pthread_join(aristotle, NULL);
}

/*************************************************
 * Function: philosopher
 * Description: The thread function for all philosopher threads. Utilizes the solution from the little book of semaphores page 93.
 * Philosophers think, get forks, eat and then put down forks.
 * Params: Argument struct pointer
 * Returns: None
 * Pre-conditions: Argument struct pointer has values initialized with data
 * Post-conditions: None
 * **********************************************/
void* philosopher(void* params)
{
    Args* args = params; //Create an argument struct pointer to fill with params
    int name = args->name; //Put the argument name into a buffer so we don't have to access it all the time (Its not a pointer and it doesn't need to change)
    while(1)
    {
        //Think
        sem_wait(args->talk); //Wait for availability of the talk semaphore for stdout and status array usage
        args->status[name] = 0; //Update status to thinking for current philosopher index
        show_status(args->status); //Print to stdout the status of every philosopher
        sem_post(args->talk); //Yield control of the talk semaphore

        sleep((prng()%20)+1); //Sleep between 1 and 20 seconds for thinking

        get_forks(name, args->footman, args->forks); //Get two adjacent forks for eating

        //Eat
        sem_wait(args->talk);
        args->status[name] = 1;
        show_status(args->status);
        sem_post(args->talk);

        sleep((prng()%8)+2); //Sleep between 2 and 9 seconds for eating

        put_forks(name, args->footman, args->forks); //Yield usage of the two adjacent forks
    }
}

/*************************************************
 * Function: show_status
 * Description: Calls on the system to clear the screen then prints to stdout the status of all philosphers
 * Params: Unsigned int array of size NUM_PHILOSOPHERS that for each element is a 0 for thinking or a 1 for eating
 * Returns: None
 * Pre-conditions: Status array has allocated memory. 
 * Post-conditions: Information has been printed to stdout
 * **********************************************/
void show_status(unsigned int* status)
{
    system("clear");
    printf("-------------------------------------------------------------\n");
    int i; for(i = 0; i < NUM_PHILOSOPHERS; i++)
    {
        show_name(i);
        if(status[i] == 0)
            printf("is thinking.\n");
        else
            printf("is eating with forks: %d and %d\n", i, right(i));
    }
    printf("-------------------------------------------------------------\n");
}

/*************************************************
 * Function: show_name
 * Description: Gets the name (Index) of a philosopher in integer form and uses this to print to stdout the name of the philosopher.
 * Params: integer between 0 and NUM_PHILOSOPHERS
 * Returns: None
 * Pre-conditions: None 
 * Post-conditions: Philosopher name is printed to stdout
 * **********************************************/
void show_name(int name)
{
    if(name == Aristotle)
        printf("[0]\tF:(0,1) - Aristotle ");
    else if(name == Socrates)
        printf("[1]\tF:(1,2) - Socrates ");
    else if(name == Plato)
        printf("[2]\tF:(2,3) - Plato ");
    else if(name == Pythagoras)
        printf("[3]\tF:(3,4) - Pythagoras ");
    else
        printf("[4]\tF:(4,0) - Democritus ");
}

/*************************************************
 * Function: get_forks
 * Description: Gets the id for a philosopher and a set of semaphores for allowing the specified philosopher exclusive access to its two adjacent forks
 * Params: integer id of philosopher, footman semaphore pointer, array of forks semaphore pointers
 * Returns: None
 * Pre-conditions: Semaphores have been allocated memory and initialized 
 * Post-conditions: Specified philosopher has exclusive access to the two adjacent forks
 * **********************************************/
void get_forks(int seat, sem_t* footman, sem_t** forks)
{
    sem_wait(footman);
    sem_wait(forks[right(seat)]);
    sem_wait(forks[seat]);
}

/*************************************************
 * Function: put_forks
 * Description: Gets the id for a philosopher and a set of semaphores for yielding exclusive access of the specified philosopher's two adjacent forks
 * Params: integer id of philosopher, footman semaphore pointer, array of forks semaphore pointers
 * Returns: None
 * Pre-conditions: Semaphores have been allocated memory and initialized 
 * Post-conditions: Exclusive access to the specified philosopher's two adjacent forks has been yielded
 * **********************************************/
void put_forks(int seat, sem_t* footman, sem_t** forks)
{
    sem_post(forks[right(seat)]);
    sem_post(forks[seat]);
    sem_post(footman);
}

/*************************************************
 * Function: right
 * Description: Gets the index position of the fork to the right of the philosopher and wraps the number since the table is circular
 * Params: Integer index of the philosopher position
 * Returns: integer index of adjacent fork to the right (i+1) % NUM_PHILOSOPHERS
 * Pre-conditions: 
 * Post-conditions: 
 * **********************************************/
int right(int i)
{
    return (i + 1) % NUM_PHILOSOPHERS;
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
