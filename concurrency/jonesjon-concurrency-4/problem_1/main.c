//Barbershop problem found on page 129 of the little book of semaphores

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct Node {
    sem_t *item;
    struct Node* next;
}Node;

typedef struct Args_t {
    sem_t* mutex, *customer, *customerDone, *barberDone, *speak;
    Node* queue;
    int* customers, *n;
}Args_t;

void insert(Node**, sem_t*);
sem_t* pop_front(Node**);

void* t_customer(void*);
void* t_barber(void*);

pthread_t* get_threads(int num_threads, void* function, void* args);
void use_stdout(const char*, sem_t*);

int main()
{
    //Initialize variables
    int *n, *customers;
    sem_t *mutex, *customer, *customerDone, *barberDone, *speak;
    Node* queue = NULL;

    n = (int*)malloc(sizeof(int));
    customers = (int*)malloc(sizeof(int));

    *n = 4;
    *customers = 0;

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

    //Create customer threads
    /*
    pthread_t *c_threads;
    c_threads = get_threads(6, t_customer, &arguments);
    */

    //Create barber thread
    pthread_t b_thread;
    pthread_create(&b_thread, NULL, t_barber, &arguments);

    pthread_t c_thread;
    //Send a new customer in every 2 seconds
    while(1)
    {
        pthread_create(&c_thread, NULL, t_customer, &arguments);
        sleep(2);
    }
    //pthread_join(b_thread, NULL);
    return 0;
}

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

sem_t* pop_front(Node** node)
{
    if((*node) == NULL)
    {
        return;
    }
    sem_t* poped_sem = (*node)->item;

    Node* temp = *node;
    *node = (*node)->next;
    free(temp);

    return poped_sem;
}

void* t_customer(void* args)
{
    //Get arguments and create a semaphore exclusive to this customer thread
    Args_t* c_args = (Args_t*)args;
    sem_t* self_sem = (sem_t*)malloc(sizeof(sem_t));
    sem_init(self_sem, 0, 0);

    sem_wait(c_args->mutex);
    if(*(c_args->customers) == *(c_args->n))
    {
        sem_post(c_args->mutex);
        //TODO: Free personal semaphore memory here
        use_stdout("[C-FULL]Barbershop is full, customer leaving.\n", c_args->speak);
        return;
    }
    *(c_args->customers) += 1;
    use_stdout("[C-WAIT] Customer is waiting in lobby.\n", c_args->speak);
    insert(&(c_args->queue), self_sem);
    sem_post(c_args->mutex);

    sem_post(c_args->customer);
    sem_wait(self_sem);

    //GET A HAIRCUT
    use_stdout("[C-HAIRCUT] Customer is getting a haircut.\n", c_args->speak);

    sem_post(c_args->customerDone);
    sem_wait(c_args->barberDone);

    sem_wait(c_args->mutex);
    *(c_args->customers) -= 1;
    use_stdout("[C-LEAVE] Customer has left the barbershop.\n", c_args->speak);
    sem_post(c_args->mutex);

    return;
}

void* t_barber(void* args)
{
    Args_t* b_args = (Args_t*)args;
    sem_t* c_sem;
    while(1)
    {
        sem_wait(b_args->customer);
        sem_wait(b_args->mutex);
        c_sem = pop_front(&(b_args->queue));
        sem_post(b_args->mutex);

        sem_post(c_sem);

        //CUT HAIR
        use_stdout("[B-CUT] Barber is cutting customer's hair.\n", b_args->speak);
        sleep(5);
        use_stdout("[B-DONE] Barber is done cutting customer's hair.\n", b_args->speak);

        sem_wait(b_args->customerDone);
        sem_post(b_args->barberDone);
        use_stdout("[B-SLEEP] Barber is taking a 5 second nap.\n", b_args->speak);
        sleep(5);
    }
    return;
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

void use_stdout(const char* msg, sem_t* sem)
{
    sem_wait(sem);
    printf(msg);
    sem_post(sem);
}