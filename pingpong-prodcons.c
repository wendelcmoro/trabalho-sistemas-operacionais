// Wendel Caio Moro GRR20182641
// João Victor Frans Pondaco Winandy GRR20182617

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ppos.h"
#include "queue.h"

#define VAGAS 5
#define NUM_PROD 3
#define NUM_CONS 2

int buffer[VAGAS], queueHead = 0, tail = 0;
semaphore_t  s_buffer, s_vaga, s_item;
task_t productors[NUM_PROD], consumers[NUM_CONS];

void produtor (void * arg)
{
    int item;
    while (1)
    {
        task_sleep(1000);

        item = rand() % 99;

        sem_down(&s_vaga);
        sem_down(&s_buffer);

        // insere item no buffer
        buffer[tail] = item;
        tail = (tail + 1) % VAGAS;

        sem_up(&s_buffer);
        sem_up(&s_item);

        //print item
        printf ("p%d produziu %d\n", (task_id() - (NUM_PROD - 1)), item);
    }

    task_exit(0);
}

void consumidor (void * arg)
{
    int item;
    while (1)
    {
        sem_down(&s_item);
        sem_down(&s_buffer);

        //retira item do buffer
        item = buffer[queueHead];
        queueHead = (queueHead + 1) % VAGAS;

        sem_up(&s_buffer);
        sem_up (&s_vaga);

        //print item
        printf ("                               c%d consumiu %d\n", (task_id() - (NUM_PROD + NUM_CONS)), item);

        task_sleep(1000);
    }

    task_exit(0);
}

int main() {
    ppos_init () ;

    printf ("main: inicio\n") ;
    
    sem_create (&s_buffer, 1);
    sem_create (&s_vaga, VAGAS);
    sem_create (&s_item, 0);

    // cria os produtores
    for (int i=0; i<NUM_PROD; i++)
        task_create (&productors[i], produtor, NULL);

    // cria os consumidores
    for (int i=0; i<NUM_CONS; i++)
        task_create (&consumers[i], consumidor, NULL);
    

    task_sleep (20);
    sem_up (&s_buffer);
    
    // aguarda as tarefas encerrarem
    for (int i=0; i<NUM_PROD; i++)
        task_join (&productors[i]);
    //sem_up (&s);

    task_exit (0);

    exit (0);
}