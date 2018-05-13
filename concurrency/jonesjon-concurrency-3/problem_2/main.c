#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "mt19937ar.h"

typedef struct Node {
	int value;
	struct Node* next;
}Node;

//Constructed from equivalent python implementation in little book of semaphores page 70
typedef struct Lightswitch { 
	int counter;
	sem_t* mutex;
}Lightswitch;

void ls_lock(Lightswitch* ls, sem_t* s)
{
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
	if(ls->counter == 0)
		sem_post(s);
	sem_post(ls->mutex);
}
//////////////////////////////////////////////////////////////////////////////////////

typedef struct Searcher_args {
	Node** list;
	Lightswitch* search_switch; 
	sem_t* no_search;
	sem_t* talk;
}Searcher_args;

typedef struct Inserter_args {
	Node** list;
	Lightswitch* insert_switch;
	sem_t* insert_mutex;
	sem_t* no_insert;
	sem_t* talk;
}Inserter_args;

typedef struct Deleter_args {
	Node** list;
	sem_t* no_search;
	sem_t* no_insert;
	sem_t* talk;
}Deleter_args;

int bit;

unsigned int prng();
void driver();
void show_list(Node*); //Searcher function
void insert(Node**, int); //Inserter function
void delete(Node**, int); //Deleter function
void delete_end(Node**);
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

	Lightswitch search_switch, insert_switch;
	sem_t *insert_mutex, *no_search, *no_insert, *talk;

	insert_mutex = (sem_t*)malloc(sizeof(sem_t));
	no_search = (sem_t*)malloc(sizeof(sem_t));
	no_insert = (sem_t*)malloc(sizeof(sem_t));
	talk = (sem_t*)malloc(sizeof(sem_t));

	search_switch.counter = 0;
	insert_switch.counter = 0;

	search_switch.mutex = (sem_t*)malloc(sizeof(sem_t));
	insert_switch.mutex = (sem_t*)malloc(sizeof(sem_t));

	//Initialize semaphores
	sem_init(insert_mutex, 0, 1);
	sem_init(no_search, 0, 1);
	sem_init(no_insert, 0, 1);
	sem_init(search_switch.mutex, 0, 1);
	sem_init(insert_switch.mutex, 0, 1);
	sem_init(talk, 0, 1);

	//Initialize arguments
	Searcher_args s_arg; 
	Inserter_args i_arg; 
	Deleter_args d_arg;
	s_arg.list = &head; s_arg.talk = talk; 
	i_arg.list = &head; i_arg.talk = talk;
	d_arg.list = &head; d_arg.talk = talk;
	s_arg.search_switch = &search_switch;
	s_arg.no_search = no_search;
	i_arg.insert_switch = &insert_switch;
	i_arg.insert_mutex = insert_mutex;
	i_arg.no_insert = no_insert;
	d_arg.no_search = no_search;
	d_arg.no_insert = no_insert;

	pthread_t *searchers, *inserters, *deleters;

	int num_searchers = prng()%5 + 1;
	int num_inserters = prng()%5 + 1;
	int num_deleters = prng()%5 + 1;

	printf("Searchers: %d\tInserters: %d\tDeleters: %d\nThread execution will begin in 5 seconds...\n", num_searchers, num_inserters, num_deleters);
	sleep(5);

	searchers = get_threads(num_searchers, searcher, &s_arg);
	inserters = get_threads(num_inserters, inserter, &i_arg);
	deleters = get_threads(num_deleters, deleter, &d_arg);

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
	//Get arguments from void*
	Searcher_args* s_arg = (Searcher_args*)(args);
	int id = pthread_self();
	while(1)
	{
		sem_wait(s_arg->talk);
		printf("[SEARCH-WAIT] Thread 0x%x is checking for active delete threads.\n", id);
		sem_post(s_arg->talk);
		ls_lock(s_arg->search_switch, s_arg->no_search);

		sem_wait(s_arg->talk);
		printf("[SEARCH-ACTION] Thread 0x%x is searching the list.\nList: ", id);
		show_list(*(s_arg->list));
		sem_post(s_arg->talk);
		sleep(prng()%3+1);

		ls_unlock(s_arg->search_switch, s_arg->no_search);
		sleep(prng()%10+1);
	}
	return;
}

void* inserter(void* args)
{
	Inserter_args* i_arg = (Inserter_args*)(args);
	int val;
	int id = pthread_self();
	while(1)
	{
		val = prng()%101;
		sem_wait(i_arg->talk);
		printf("[INSERT-WAIT] Thread 0x%x is checking for active insert and delete threads.\n", id);
		sem_post(i_arg->talk);
		ls_lock(i_arg->insert_switch, i_arg->no_insert);
		sem_wait(i_arg->insert_mutex);

		insert(i_arg->list, val);
		sem_wait(i_arg->talk);
		printf("[INSERT-ACTION] Thread: 0x%x inserted %d into the list.\n", id, val);
		sem_post(i_arg->talk);

		sleep(prng()%3+1);

		sem_post(i_arg->insert_mutex);
		ls_unlock(i_arg->insert_switch, i_arg->no_insert);
		sleep(prng()%10+1);

	}
	return;
}

void* deleter(void* args)
{
	Deleter_args* d_arg = (Deleter_args*)(args);
	int id = pthread_self();
	while(1)
	{
		sem_wait(d_arg->talk);
		printf("[DELETE-WAIT] Thread 0x%x is checking for active search, insert and delete threads.\n", id);
		sem_post(d_arg->talk);
		sem_wait(d_arg->no_search);
		sem_wait(d_arg->no_insert);

		delete_end(d_arg->list);
		sem_wait(d_arg->talk);
		printf("[DELETE-ACTION] Thread 0x%x deleted end of list.\n", id);
		sem_post(d_arg->talk);
		sleep(prng()%3+1);

		sem_post(d_arg->no_insert);
		sem_post(d_arg->no_search);
		sleep(prng()%10+1);
	}
	return;
}

void show_list(Node* node)
{
	if(node != NULL)
	{
		printf("%d, ", node->value);
		show_list(node->next);
		return;
	}
	printf("\n");
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
			return;
		}
	}
	else if((*node)->value == value) //If current node we're at is the head
	{
		//Set the new head and free the current head
		Node* temp = (*node)->next;
		*node = (*node)->next;
		free(temp);
		return;
	}
	delete(&(*node)->next, value);
}

void delete_end(Node** node)
{
	if(*node == NULL) //Empty list
		return;

	if((*node)->next != NULL) //Is the next item null?
	{
		if((*node)->next->next == NULL) //Next item is the tail
		{
			free((*node)->next);
			(*node)->next = NULL;
			return;
		}
	}
	else { //We're the head of the list
		free(*node);
		*node = NULL;
		return;
	}
	delete_end(&(*node)->next);
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