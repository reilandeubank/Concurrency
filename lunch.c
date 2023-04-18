// Reiland Eubank
// CS 300
// Project 3



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include "mytime.h"
#include <semaphore.h>

#define MAX_LIST_SIZE 50      // Max buffer size

int i = 0;

void Show_serving(int number) {
    printf("Serving %d\n", number);
}

struct lunch 
{
    int myTicketNum;
    int *ticket_queue; // FIFO queue to store ticket numbers
    int head; // index of the next ticket in ticket_queue to be served
    int tail; // index of last slot in ticket_queue
    int ticket_counter; // ticket counter
    int current_ticket; // current ticket being served
    sem_t ticket_queue_sem; // semaphore to protect access to the ticket queue
    sem_t customer_sem; // semaphore to indicate presence of a customer
    sem_t server_sem;  // semaphore to indicate presence of a server
    sem_t sem_array[MAX_LIST_SIZE];
    pthread_mutex_t current_ticket_lock;    // lock to protect access to the ticket queue
};

void lunch_init(struct lunch *lunch) {
    lunch->myTicketNum = 0;
    lunch->head = 0;
    lunch->tail = 0;
    lunch->ticket_counter = 1;
    lunch->current_ticket = -1;
    lunch->ticket_queue = malloc(MAX_LIST_SIZE * sizeof(int));
    sem_init(&(lunch->ticket_queue_sem), 0, 1);
    sem_init(&(lunch->customer_sem), 0, 0);
    sem_init(&(lunch->server_sem), 0, 0);
    pthread_mutex_init(&lunch->current_ticket_lock, NULL);
    // initialize the queue and arrray to all zeros
    memset(lunch->ticket_queue, 0, MAX_LIST_SIZE * sizeof(int));

    for (i = 0; i < MAX_LIST_SIZE; i++) {
        sem_init(&(lunch->sem_array[i]), 0, 0);
    }
}

int lunch_get_ticket(struct lunch *lunch) {
    printf("<%lu Customer> enter <lunch_get_ticket>\n", pthread_self());
    sem_wait(&lunch->ticket_queue_sem);

    int ticket_number = lunch->ticket_counter++
    printf("<%lu Customer> get ticket %d\n", pthread_self(), ticket_number);

    sem_post(&lunch->ticket_queue_sem);

    printf("<%lu Customer> leave <lunch_get_ticket>\n", pthread_self());

    return ticket_number;
}

void lunch_wait_turn(struct lunch *lunch, int ticketNum) {
    int ticket = ticketNum;
    printf("<%lu Customer> enter lunch_wait_turn with ticket %d\n", pthread_self(), ticket);

    sem_wait(&lunch->ticket_queue_sem);
    lunch->ticket_queue[lunch->tail] = ticket;
    lunch->tail = (lunch->tail + 1) % MAX_LIST_SIZE;
    sem_post(&lunch->ticket_queue_sem);

    sem_post(&lunch->customer_sem);
    sem_wait(&lunch->server_sem);

    sem_wait(&lunch->sem_array[ticket]);
    printf("<%lu Customer> leave lunch_wait_turn after ticket %d served.\n", pthread_self(), ticket);
    sem_post(&lunch->sem_array[ticket]);
}

void lunch_wait_customer(struct lunch *lunch) {
    int ticket;
    printf("<%ld Server> enter lunch_wait_customer\n", (long) pthread_self());
    
    sem_post(&lunch->server_sem);
    sem_wait(&lunch->customer_sem);

    sem_wait(&lunch->ticket_queue_sem);
    if(lunch->head != lunch->tail) {
        pthread_mutex_lock(&lunch->current_ticket_lock);
        lunch->current_ticket = lunch->ticket_queue[lunch->head];
        lunch->head = (lunch->head + 1) % MAX_LIST_SIZE;
        Show_serving(lunch->current_ticket);
        pthread_mutex_unlock(&lunch->current_ticket_lock);

        sem_post(&lunch->sem_array[lunch->current_ticket]);
        usleep(10);
        sem_wait(&lunch->sem_array[lunch->current_ticket]);
        printf("<%ld Server> after served ticket %d\n", (long) pthread_self(), lunch->current_ticket);
        sem_post(&lunch->sem_array[lunch->current_ticket]);
    }
    sem_post(&lunch->ticket_queue_sem);

    printf("<%lu Server> leave lunch_wait_customer\n", pthread_self());
}

void *server(void *arg) {
    struct lunch *lunch = (struct lunch *)arg;
    int val = mytime(5, 10);
    printf("Sleeping Time: %d sec; Thread Id = %ld\n", val, (long) pthread_self());
    sleep(val);
    lunch_wait_customer(lunch);
    return NULL;
}

void *customer(void *arg) {
    struct lunch *lunch = (struct lunch *)arg;
    int tix = lunch_get_ticket(lunch);
    lunch->myTicketNum = tix;
    int val = mytime(0, 5);
    printf("Sleeping Time: %d sec; Thread Id = %ld\n", val, (long) pthread_self());
    sleep(val);
    lunch_wait_turn(lunch, tix);
    return NULL;
}

int main(int argc, char **argv) {
    printf("here\n");
    int total_customers, total_servers;
    if (argc != 3) {
        printf("Usage: %s <total_customers> <total_servers>, please try again\n", argv[0]);
        scanf("%d %d", &total_servers, &total_customers);
    }
    else {
        total_servers = atoi(argv[1]);
        total_customers = atoi(argv[2]);
    }
    if (total_customers <= 0 || total_servers <= 0) {
        printf("Error: total_customers and total_servers must be positive integers, please try again\n");
        scanf("%d %d", &total_servers, &total_customers);
    }

    struct lunch lunch;
    lunch_init(&lunch);

    pthread_t customers[total_customers];            // create customer threads
    for (i = 0; i < total_customers; i++) {
        pthread_create(&customers[i], NULL, customer, &lunch);
    }
    pthread_t servers[total_servers];                // create server threads
    for (i = 0; i < total_servers; i++) {
        pthread_create(&servers[i], NULL, server, &lunch);
    }

    for (i = 0; i < total_customers; i++) {
        pthread_join(customers[i], NULL);
    }
    for (i = 0; i < total_servers; i++) {
        pthread_join(servers[i], NULL);
    }
    return 0;
}