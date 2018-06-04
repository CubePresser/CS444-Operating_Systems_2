////////////////////////////////////////////////////////
// Jonathan Jones - jonesjon 932709446
// Concurrency 4
// CS444 Spring2018
////////////////////////////////////////////////////////
//Barbershop problem found on page 129 of the little book of semaphores

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

//For a linked list of semaphores
typedef struct Node {
    sem_t *item;
    struct Node* next;
}Node;

//Thread arguments
typedef struct Args_t {
    sem_t* mutex, *customer, *customerDone, *barberDone, *speak;
    Node* queue;
    int* customers, *n, *total_customers;
}Args_t;

//Function prototypes
void insert(Node**, sem_t*);
sem_t* pop_front(Node**);

void* t_customer(void*);
void get_hair_cut(sem_t*, int);
void* t_barber(void*);
void cut_hair(sem_t*);

pthread_t* get_threads(int num_threads, void* function, void* args);
void use_stdout(const char*, sem_t*, int);

int main(int argc, char** argv)
{
    //Check usage
    if(argc < 2)
    {
        printf("USAGE: main <NUM_CHAIRS>\n");
        exit(1);
    }

    //Initialize variables
    int *n, *customers, *total_customers;
    sem_t *mutex, *customer, *customerDone, *barberDone, *speak;
    Node* queue = NULL;

    n = (int*)malloc(sizeof(int));
    customers = (int*)malloc(sizeof(int));
    total_customers = (int*)malloc(sizeof(int));

    *n = atoi(argv[1]);
    *customers = 0;
    *total_customers = 0;

    mutex = (sem_t*)malloc(sizeof(sem_t));
    customer = (sem_t*)malloc(sizeof(sem_t));
    customerDone = (sem_t*)malloc(sizeof(sem_t));
    barberDone = (sem_t*)malloc(sizeof(sem_t));
    speak = (sem_t*)malloc(sizeof(sem_t));

    sem_init(mutex, 0, 1);
    sem_init(customer, 0, 0);
    sem_init(customerDone, 0, 0);
    sem_init(barberDone, 0, 0);
    sem_init(speak, 0, 1);

    Args_t arguments;
    arguments.mutex = mutex;
    arguments.customer = customer;
    arguments.customerDone = customerDone;
    arguments.barberDone = barberDone;
    arguments.speak = speak;
    arguments.queue = queue;
    arguments.customers = customers;
    arguments.n = n;
    arguments.total_customers = total_customers;

    //Create barber thread
    pthread_t b_thread;
    pthread_create(&b_thread, NULL, t_barber, &arguments);

    //Send a new customer in every 4 seconds
    pthread_t c_thread;
    while(1)
    {
        pthread_create(&c_thread, NULL, t_customer, &arguments);
        sleep(4);
    }
    return 0;
}

/*************************************************
 * Function: insert
 * Description: Recursively insert a node at the end of the linked list
 * Params: Address to a node pointer (If we need to update the head), Address of semaphore we want to add
 * Returns: None
 * Pre-conditions: None 
 * Post-conditions: Node has been added to the end of the linked list with allocated memory or head has been added
 * **********************************************/
void insert(Node** node, sem_t *sem)
{
    if((*node) == NULL)
    {
        (*node) = (Node*)malloc(sizeof(Node));
        (*node)->next = NULL;
        (*node)->item = sem;
        return;
    }
    insert(&((*node)->next), sem);
}

/*************************************************
 * Function: pop_front
 * Description: Removes a node from the head of the linked list and returns the address of the semaphore associated with it
 * Params: Address to head pointer.
 * Returns: Address of the semaphore in the head node.
 * Pre-conditions: List should have at least one node in it otherwise this returns NULL as an error
 * Post-conditions: Address of semaphore or NULL has been returned.
 * **********************************************/
sem_t* pop_front(Node** node)
{
    if((*node) == NULL)
    {
        return NULL;
    }
    sem_t* poped_sem = (*node)->item;

    Node* temp = *node;
    *node = (*node)->next;
    free(temp);

    return poped_sem;
}

/*************************************************
 * Function: t_customer
 * Description: Customer thread function. Enters barbershop if it is not full and waits in line to get a haircut.
 * Params: Args_t struct for semaphores, queue and counters
 * Returns: None
 * Pre-conditions: Args_t struct has been initialized and filled with values. 
 * Post-conditions: Customer has left shop if full or left shop after getting a haircut
 * **********************************************/
void* t_customer(void* args)
{
    //Get arguments and create a semaphore exclusive to this customer thread
    Args_t* c_args = (Args_t*)args;
    sem_t* self_sem = (sem_t*)malloc(sizeof(sem_t));
    sem_init(self_sem, 0, 0);

    //Wait for exclusive access to resources
    sem_wait(c_args->mutex);
    if(*(c_args->customers) == *(c_args->n)) //If the number of customers is equal to the number of chairs
    {
        sem_post(c_args->mutex); //Give up exclusive access and leave barbershop
        use_stdout("[C-FULL]Barbershop is full, customer leaving.\n", c_args->speak, 0);
        return;
    }

    *(c_args->customers) += 1; //Update number of customers and customer count
    *(c_args->total_customers) += 1;
    int customer_id = *(c_args->total_customers);

    use_stdout("[C-WAIT] Customer is waiting in lobby.\n", c_args->speak, customer_id);
    insert(&(c_args->queue), self_sem); //Insert this customer into the queue
    sem_post(c_args->mutex); //Give up exclusive access

    sem_post(c_args->customer);
    sem_wait(self_sem); //Wait until this thread has been signaled for a haircut by the barber

    get_hair_cut(c_args->speak, customer_id); //Get a hair cut

    sem_post(c_args->customerDone);
    sem_wait(c_args->barberDone);

    sem_wait(c_args->mutex);
    *(c_args->customers) -= 1; //Decrement number of customers currently waiting
    use_stdout("[C-LEAVE] Customer has left the barbershop.\n", c_args->speak, customer_id);
    sem_post(c_args->mutex);

    return;
}

/*************************************************
 * Function: get_hair_cut
 * Description: Sleeps for 5 seconds and tells grader.
 * Params: Speaking semaphore, customer id
 * Returns: None
 * Pre-conditions: None 
 * Post-conditions: None
 * **********************************************/
void get_hair_cut(sem_t* speak, int id)
{
    use_stdout("[C-HAIRCUT] Customer is getting a haircut for 5 seconds.\n", speak, id);
    sleep(5);
}

/*************************************************
 * Function: t_barber
 * Description: Gives customers haircuts and signals them for a haircut in the order that they arrive in the waiting room
 * Params: Args_t struct for semaphores, queue and counters
 * Returns: None
 * Pre-conditions: Args_t struct has been initialized, memory allocated and values set
 * Post-conditions: None, never exits.
 * **********************************************/
void* t_barber(void* args)
{
    //Initialize arguments
    Args_t* b_args = (Args_t*)args;
    sem_t* c_sem;

    while(1)
    {
        sem_wait(b_args->customer); //Wait for a customer
        sem_wait(b_args->mutex); //Get exclusive resource access
        c_sem = pop_front(&(b_args->queue)); //Get the first customer in the queue
        sem_post(b_args->mutex); //Give up exclusive access

        sem_post(c_sem);

        cut_hair(b_args->speak); //Cut hair
        use_stdout("[B-DONE] Barber is done cutting customer's hair.\n", b_args->speak, 0);

        sem_wait(b_args->customerDone);
        sem_post(b_args->barberDone);
        use_stdout("[B-SLEEP] Barber is taking a 3 second nap.\n", b_args->speak, 0); //Take a 3 second nap
        sleep(3);
    }
    return;
}

/*************************************************
 * Function: cut_hair
 * Description: Informs grader that the barber is cutting hair and sleeps for 5 seconds (same duration as get_hair_cut)
 * Params: Speak semaphore
 * Returns: none
 * Pre-conditions: none
 * Post-conditions: none
 * **********************************************/
void cut_hair(sem_t* speak)
{
    use_stdout("[B-CUT] Barber is cutting customer's hair for 5 seconds.\n", speak, 0);
    sleep(5);
}

/*************************************************
 * Function: get_threads
 * Description: Creates and returns a certain number of threads given the function and arguments to use
 * Params: number of threads (greater than 0), void* function for the thread to use, arguments to that function in a structure
 * Returns: List of pthreads.
 * Pre-conditions: Valid arguments passed in, non-negative num_threads
 * Post-conditions: Threads are initialized properly and executing.
 * **********************************************/
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
 * Function: use_stdout
 * Description: Grants exclusive access to stdout and prints a message to the screen with ID of who is speaking if necessary.
 * Params: Message, speaking semaphore, id
 * Returns: None
 * Pre-conditions: Speaking semaphore is set up, id is 0 if no ID is to be displayed 
 * Post-conditions: None
 * **********************************************/
void use_stdout(const char* msg, sem_t* sem, int id)
{
    sem_wait(sem);
    if(id == 0)
        printf(msg);
    else
        printf("[C-#%d]---%s", id, msg);
    sem_post(sem);
}