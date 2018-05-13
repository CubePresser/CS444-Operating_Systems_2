#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "mt19937ar.h"

typedef struct Node {
	int value;
	struct Node* next;
}Node;

typedef struct searcher_args {
	Node* list;
	sem_t* is_deleting;
}searcher_args;

typedef struct inserter_args {
	Node* list;
	sem_t* is_deleting;
	sem_t* is_inserting;
}inserter_args;

typedef struct deleter_args {
	Node* list;
	sem_t* is_deleting;
}deleter_args;

int bit;

unsigned int prng();
void driver();
void show_list(Node*); //Searcher function
void insert(Node**, int); //Inserter function
void delete(Node**, int); //Deleter function
void free_list(Node*);

void* searcher(void*);
void* inserter(void*);
void* deleter(void*);

pthread_t* get_threads(int, void*, void*);

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
	//Initialize constructs for problem
	Node* head = NULL;
	sem_t* is_deleting, *is_inserting;
	is_deleting = (sem_t*)malloc(sizeof(sem_t));
	is_inserting = (sem_t*)malloc(sizeof(sem_t));

	pthread_t* searchers, *inserters, *deleters;

	int num_searchers = prng()%5 + 1;
	int num_inserters = prng()%5 + 1;
	int num_deleters = prng()%5 + 1;

	searchers = get_threads(num_searchers, searcher, NULL);
	inserters = get_threads(num_inserters, inserter, NULL);
	deleters = get_threads(num_deleters, deleter, NULL);

	//printf("S: %d\nI: %d\nD: %d\n", num_searchers, num_inserters, num_deleters);

	pthread_join(searchers[0], NULL);

	return;
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

void* searcher(void* args)
{
	return;
}

void* inserter(void* args)
{
	return;
}

void* deleter(void* args)
{
	return;
}

void show_list(Node* node)
{
	if(node != NULL)
	{
		printf("%d\n", node->value);
		show_list(node->next);
	}
}

void insert(Node** node, int value)
{
	if(*node == NULL)
	{
		*node = (Node*)malloc(sizeof(Node));
		(*node)->value = value;
		(*node)->next = NULL;
		return;
	}
	insert(&(*node)->next, value);
}

void delete(Node** node, int value)
{
	if(*node == NULL) //If list is empty or end of list reached
		return;
	
	if((*node)->next != NULL)
	{
		if((*node)->next->value == value) //If the next node over has the value we want
		{
			Node* temp = (*node)->next; //Temporarily store the location of the node we're removing
			(*node)->next = temp->next; //Direct current node to whatever follows the deleted node
			free(temp); //Free the memory that temp points to
		}
	}
	else if((*node)->value == value) //If current node we're at is the head
	{
		//Set the new head and free the current head
		Node* temp = *node;
		*node = (*node)->next;
		free(temp);
	}
	delete(&(*node)->next, value);
}

void free_list(Node* node)
{
	if(node == NULL)
		return;
	free_list(node->next);
	free(node);
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