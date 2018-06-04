////////////////////////////////////////////////////////
// Jonathan Jones - jonesjon 932709446
// Concurrency 4
// CS444 Spring2018
////////////////////////////////////////////////////////
//Solution on page 109 of the little book of semaphores

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "mt19937ar.h"

//Arguments struct for agent threads
typedef struct Agent_args {
    sem_t *agentSem, *item1, *item2, *speak;
    const char* name;
}Agent_args;

//Arguments struct for pusher threads
typedef struct Pusher_args {
    sem_t* mutex, *item1, *item2, *item3, *speak;
    int* b_item1, *b_item2, *b_item3;
    const char* name;
}Pusher_args;

//Arguments struct for smoker threads
typedef struct Smoker_args {
    sem_t* item, *agentSem, *speak;
    const char* name;
}Smoker_args;

//Function prototypes
void driver();
unsigned int prng();

void* agent_thread(void*);
void* pusher_thread(void*);
void* smoker_thread(void*);

//Keeps track of what sort of random number generator method should be used
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
    //Initialize variables
    sem_t *agentSem, *tobacco, *paper, *match, *tobaccoSem, *paperSem, *matchSem, *mutex, *speak;
    int *isTobacco, *isPaper, *isMatch;

    //Allocate memory for variables
    agentSem = (sem_t*)malloc(sizeof(sem_t));
    tobacco = (sem_t*)malloc(sizeof(sem_t));
    paper = (sem_t*)malloc(sizeof(sem_t));
    match = (sem_t*)malloc(sizeof(sem_t));
    tobaccoSem = (sem_t*)malloc(sizeof(sem_t));
    paperSem = (sem_t*)malloc(sizeof(sem_t));
    matchSem = (sem_t*)malloc(sizeof(sem_t));
    mutex = (sem_t*)malloc(sizeof(sem_t));
    speak = (sem_t*)malloc(sizeof(sem_t));

    isTobacco = (int*)malloc(sizeof(int));
    isPaper = (int*)malloc(sizeof(int));
    isMatch = (int*)malloc(sizeof(int));

    //Initialize variable values
    sem_init(agentSem, 0, 1);
    sem_init(tobacco, 0, 0);
    sem_init(paper, 0, 0);
    sem_init(match, 0, 0);
    sem_init(tobaccoSem, 0, 0);
    sem_init(paperSem, 0, 0);
    sem_init(matchSem, 0, 0);
    sem_init(mutex, 0, 1);
    sem_init(speak, 0, 1);

    *isTobacco = *isPaper = *isMatch = 0;

    //Set up agent arguments
    Agent_args agent_a, agent_b, agent_c;
    agent_a.agentSem = agentSem; agent_a.speak = speak;
    agent_a.item1 = tobacco; agent_a.item2 = paper;
    agent_a.name = "tobacco and paper";

    agent_b.agentSem = agentSem; agent_b.speak = speak;
    agent_b.item1 = paper; agent_b.item2 = match;
    agent_b.name = "paper and matches";

    agent_c.agentSem = agentSem; agent_c.speak = speak;
    agent_c.item1 = tobacco; agent_c.item2 = match;
    agent_c.name = "tobacco and matches";

    //Set up pusher arguments
    Pusher_args pusher_a, pusher_b, pusher_c;
    pusher_a.mutex = mutex; pusher_a.speak = speak;
    pusher_a.item1 = match; pusher_a.item2 = tobaccoSem; pusher_a.item3 = paperSem;
    pusher_a.b_item1 = isMatch; pusher_a.b_item2 = isTobacco; pusher_a.b_item3 = isPaper;
    pusher_a.name = "matches";

    pusher_b.mutex = mutex; pusher_b.speak = speak;
    pusher_b.item1 = tobacco; pusher_b.item2 = paperSem; pusher_b.item3 = matchSem;
    pusher_b.b_item1 = isTobacco; pusher_b.b_item2 = isPaper; pusher_b.b_item3 = isMatch;
    pusher_b.name = "tobacco";

    pusher_c.mutex = mutex; pusher_c.speak = speak;
    pusher_c.item1 = paper; pusher_c.item2 = tobaccoSem; pusher_c.item3 = matchSem;
    pusher_c.b_item1 = isPaper; pusher_c.b_item2 = isTobacco; pusher_c.b_item3 = isMatch;
    pusher_c.name = "paper";

    //Set up smoker arguments
    Smoker_args smoker_a, smoker_b, smoker_c;
    smoker_a.item = matchSem;
    smoker_a.agentSem = agentSem;
    smoker_a.speak = speak;
    smoker_a.name = "Smoker with matches";

    smoker_b.item = tobaccoSem;
    smoker_b.agentSem = agentSem;
    smoker_b.speak = speak;
    smoker_b.name = "Smoker with tobacco";

    smoker_c.item = paperSem;
    smoker_c.agentSem = agentSem;
    smoker_c.speak = speak;
    smoker_c.name = "Smoker with paper";

    printf("Standby while the distributors gather their goods...\n");

    //Set up threads
    pthread_t t_agent_a, t_agent_b, t_agent_c, t_pusher_a, t_pusher_b, t_pusher_c, t_smoker_a, t_smoker_b, t_smoker_c;
    pthread_create(&t_agent_a, NULL, agent_thread, &agent_a);
    pthread_create(&t_agent_b, NULL, agent_thread, &agent_b);
    pthread_create(&t_agent_c, NULL, agent_thread, &agent_c);
    pthread_create(&t_pusher_a, NULL, pusher_thread, &pusher_a);
    pthread_create(&t_pusher_b, NULL, pusher_thread, &pusher_b);
    pthread_create(&t_pusher_c, NULL, pusher_thread, &pusher_c);
    pthread_create(&t_smoker_a, NULL, smoker_thread, &smoker_a);
    pthread_create(&t_smoker_b, NULL, smoker_thread, &smoker_b);
    pthread_create(&t_smoker_c, NULL, smoker_thread, &smoker_c);

    pthread_join(t_agent_a, NULL); //Wait agent a's thread to end (it never does)

    return;
}

/*************************************************
 * Function: agent_thread
 * Description: Signals two of the three cigarette ingredient semaphores for usage by a smoker.
 * Params: Address of an Agent_args struct.
 * Returns: none
 * Pre-conditions: Agent_args struct is completely initialized with memory allocated for all necessary items.
 * Post-conditions: none
 * **********************************************/
void* agent_thread(void* args)
{
    Agent_args* a_args = (Agent_args*)args; //Reformat void* argument into Agent_args struct
    while(1)
    {
        sleep(prng()%10 + 1); //Sleep between 1-10 seconds
        sem_wait(a_args->agentSem); //Wait for agent exclusive access
        
        //Signal items
        sem_post(a_args->item1);
        sem_post(a_args->item2);

        sem_wait(a_args->speak);
        printf("Agent with %s has distributed their goods.\n", a_args->name);
        sem_post(a_args->speak);
    }
}

/*************************************************
 * Function: pusher_thread
 * Description: Determines which smoker to signal based on the order of the actions of the other pushers.
 * Params: Address of a Pusher_args struct.
 * Returns: none
 * Pre-conditions: Pusher_args struct is completely initialized with memory allocated fro all necessary items.
 * Post-conditions: none
 * **********************************************/
void* pusher_thread(void* args)
{
    Pusher_args* p_args = (Pusher_args*)args; //Reformat void* argument into Pusher_args struct

    while(1)
    {
        sem_wait(p_args->item1); //Wait for this thread's item to be signaled
        sem_wait(p_args->mutex); //Gain exclusive access to the shared thread resources
        if(*(p_args->b_item2)) //If item2 is available, signal the smoker that has item3
        {
            *(p_args->b_item2) = 0;
            sem_post(p_args->item3);
        }
        else if(*(p_args->b_item3)) //If item3 is available, signal the smoker that has item2
        {
            *(p_args->b_item3) = 0;
            sem_post(p_args->item2);
        }
        else //If this pusher is the first one to be called, set its item status to true and yield to the next pusher
        {
            *(p_args->b_item1) = 1;
        }
        sem_post(p_args->mutex); //Release access of resources
    }
}

/*************************************************
 * Function: smoker_thread
 * Description: Waits until the ingredients complementary to its given ingredient are signaled then "uses" them.
 * Params: Address of a Smoker_args struct.
 * Returns: none
 * Pre-conditions: Smoker_args struct is completely initialized with memory allocated fro all necessary items.
 * Post-conditions: none
 * **********************************************/
void* smoker_thread(void* args)
{
    Smoker_args* s_args = (Smoker_args*)args; //Reformat void* argument into Smoker_args struct

    while(1)
    {
        sem_wait(s_args->item); //Wait for the signal to start making a cigarette

        sem_wait(s_args->speak);
        printf("%s is making a cigarette.\n", s_args->name); //Make a cigarette
        sem_post(s_args->speak);

        sem_post(s_args->agentSem); //Signal the agents that the ingredients have been taken
        sleep(prng()%10 + 1); //Make the cigarette for some time

        sem_wait(s_args->speak);
        printf("%s is smoking.\n", s_args->name); //Smoke
        sem_post(s_args->speak);

        sleep(prng()%10 + 1); //Smoke for some time
    }
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