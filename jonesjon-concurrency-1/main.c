// CONCURRENCY 1 - OPERATING SYSTEMS II
// Jonathan Alexander Jones
// ------------------------------------
// This program is an implementation of the producer consumer problem solution found in The Little Book of Semaphores, page 65.
// Uses threads to concurrently process and consume data being fed into a buffer.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include "mt19937ar.h"

/*
 * SOME NOTES:
 *
 * Threads must have EXCLUSIVE access to the buffer when they want to use it. Don't want a race condition.
 * Consumer thread must wait (block) at the buffer if it is empty until the producer fills it
 * If producer wants to add an item to a FULL buffer it will block until a consumer removes an item
 *
 */

//Holds a value and a time for consumer to wait
typedef struct Item {
    unsigned int value;
    unsigned int time;
} Item;


//Globals
unsigned int bit; //0 for mt19937, 1 for rdrand
int size = 0; //Keeps track of the current index position in buffer
sem_t mutex, items, spaces; //Semaphores to be used between the producer and consumer threads
Item buffer[32]; //Buffer to hold Items

//Function prototypes
unsigned int prng();
void driver();
void* consumer(void*);
void* producer(void*);

int main(int argc, char **argv)
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

    driver();

    return 0;
}

/*************************************************
 * Function: driver
 * Description: Intializes semaphores and threads then runs the threads. Starts to wait for the consumer thread which never ends.
 * Params: None
 * Returns: None
 * Pre-conditions: Globals are initialized
 * Post-conditions: None
 * **********************************************/
void driver()
{
    //Initialize semaphores like the textbook solution
    sem_init(&items, 0, 0); //Keeps track of the number of consumers queued up and number of items
    sem_init(&mutex, 0, 1); //Used to regulate exclusive access to the buffer and the size of the buffer values
    sem_init(&spaces, 0, 31); //Keeps track of the size of the buffer (decrements the value until it reaches zero)

    //Initialize threads
    pthread_t p_thread, c_thread;
    pthread_create(&p_thread, NULL, producer, NULL);
    pthread_create(&c_thread, NULL, consumer, NULL);
    //Block parent thread until a thread completes (They never will)
    pthread_join(c_thread, NULL);
}

/*************************************************
 * Function: producer
 * Description: The producer thread function. Generates a random Item object then uses semaphores to block itself until it is given access to the buffer.
 * Once access to the buffer is acquired, it adds the item to the buffer then signals the consumer thread that it is done using the buffer.
 * The producer will block if the buffer is "full" with 32 Items until the consumer "removes" an item.
 * Params: NULL
 * Returns: None
 * Pre-conditions: Threads and semaphores have been properly initialized.
 * Post-conditions: None 
 * **********************************************/
void* producer(void* empty)
{
    Item p_item;
    unsigned int p_wait;
    while(1)
    {
        //Generate random values for the Item to be placed in the buffer
        p_item.value = prng();
        p_item.time = (prng()%8)+2;

        sem_wait(&spaces); //Check if spaces is in use and increment it by 1

        //Thread has priority from here
        sem_wait(&mutex);
        buffer[size] = p_item; //Add item to buffer
        size++; //Increment size
        p_wait = (prng()%5)+3; //Generate a random number between 3 and 7 to sleep for
        printf("[ P ] -- Producer added item:\nValue = 0x%x\nProcess Time = %ds\nCurrent buffer size (highest index value): %d\n\n", p_item.value, p_wait, size);
        sem_post(&mutex);
        //Thread priority ended

        sem_post(&items);
        sleep(p_wait); //Wait for p_wait seconds 
    }

    return;
}

/*************************************************
 * Function: consumer
 * Description: The consumer thread function. Uses semaphores to block itself until it is given access to the buffer. Once access to the buffer is acquired,
 * it removes an item from the buffer, decremenets the size then signals the producer thread that it is done using the buffer.
 * The consumer will block if the buffer is "empty" with 0 items until the producer "adds" an item.
 * Params: NULL
 * Returns: None
 * Pre-conditions: Threads and semaphores have been properly initialized.
 * Post-conditions: None
 * **********************************************/
void* consumer(void* empty)
{
    Item c_item;
    while(1)
    {
        sem_wait(&items);

        //Thread priority starts here
        sem_wait(&mutex); //Wait until the producer has finished modifying the buffer and size
        c_item = buffer[size-1]; //Remove item at last filled index
        size--; //Decrement size
        printf("[ C ] -- Consumer removed item:\nValue = 0x%x\nProcess Time = %ds\nCurrent buffer size (highest index value): %d\n\n", c_item.value, c_item.time, size);
        sem_post(&mutex);
        //Thread priority ended

        sem_post(&spaces);
        sleep(c_item.time); //Wait for a random amount of seconds determined in the producer thread
    }

    return;
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
