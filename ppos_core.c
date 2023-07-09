// Wendel Caio Moro GRR20182641
// João Victor Frans Pondaco Winandy GRR20182617

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include "ppos.h"
#include <sys/time.h>
#include <signal.h>

//#define DEBUG
#define STACKSIZE 32768
#define QUANTUM 20
#define SIZEOFDOUBLE (sizeof(double))
#define SIZEOFINT (sizeof(int))

task_t *currentTask = NULL, *lastTask = NULL, dispatch, mainTask;
ucontext_t firstTask;
int id, taskNumber = 0;
task_t *tasks, *sleepingTasks;
bool mainSwitch = false, dispatchControl = false;
long totalExecutionTime=0, nextWake = -1; // Total Execution time of tasks in milliseconds
int lock = 0;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização to timer
struct itimerval timer ;

void enter_cs ()    // Stops time preemption
{
    lock = 1;
}

void leave_cs ()    // Return time preemption
{
    lock = 0 ;
}

int task_create(task_t *task, void (*start_func)(void *), void *arg)
{
    char *stack;
    getcontext(&task->context);

    stack = malloc(sizeof(task_t) + STACKSIZE);
    if (stack)
    {
        task->next = NULL;
        task->prev = NULL;
        task->id = taskNumber;
        task->staticPriority = 0;
        task->dynamicPriority = 0;
        task->quantum = QUANTUM;
        task->waitingTasks = NULL;
        task->running = true;
        taskNumber++;

        enter_cs();
        if (task->id != 0) queue_append((queue_t **) &tasks, (queue_t *) task);     // Dispatcher shouldn't be in the queue
        leave_cs();

        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    }
    else
    {
        return (-1);
    }

    #ifdef DEBUG
        printf ("task_create: criou tarefa %d\n", task->id) ;
    #endif

    makecontext(&task->context, (void *)(*start_func), 1, (void *)arg);

    return (0);
}

int task_switch(task_t *task)
{
    enter_cs();

    id = task->id;
    if (mainSwitch == false)
    {
        mainSwitch = true;
        lastTask = currentTask;
        currentTask = task;

        currentTask->activations++;

        #ifdef DEBUG
                if (lastTask != NULL) printf ("task_switch: trocando contexto %d -> %d\n", lastTask->id, task->id) ;
                    else printf ("task_switch: trocando contexto 0 -> %d\n", task->id) ;
        #endif

        swapcontext(&mainTask.context, &task->context);

        mainSwitch = false;
    }
    else
    {
        lastTask = currentTask;

        #ifdef DEBUG
                if (lastTask != NULL) printf ("task_switch: trocando contexto %d -> %d\n", lastTask->id, task->id) ;
                        else printf ("task_switch: trocando contexto 0 -> %d\n", task->id) ;
        #endif

        currentTask = task;
        currentTask->activations++;

        swapcontext(&lastTask->context, &currentTask->context);
    }
    leave_cs();
    return (0);
}

void task_exit(int exit_code)
{
    printf ("Task %d exit: execution time %ld ms, processor time  %ld ms, %d activations\n", currentTask->id, 
                                                                     totalExecutionTime, currentTask->executionTime, currentTask->activations);
    enter_cs();
    while (queue_size((queue_t *) currentTask->waitingTasks) > 0) {
        queue_t *aux = queue_remove((queue_t **) &currentTask->waitingTasks, (queue_t *) currentTask->waitingTasks);
        queue_append((queue_t **) &tasks, aux);
    }

    if (queue_size((queue_t *) tasks) > 0) queue_remove((queue_t **) &tasks, (queue_t *) currentTask);
    currentTask->running = false;
    currentTask->exitCode = exit_code;

    leave_cs();
    if (dispatchControl && currentTask != &dispatch) task_switch(&dispatch);
    else setcontext(&mainTask.context);
    
    #ifdef DEBUG
        printf ("task_exit: tarefa %d sendo encerrada\n", currentTask->id) ;
    #endif
}

int task_id()
{
    return (id);
}

task_t *scheduler()
{
    if (tasks == NULL)
        return NULL;

    task_t *first = tasks;
    task_t *nextTask = first;
    task_t *tryTask = first->next;

    while (tryTask != first) {
        if (tryTask->dynamicPriority < nextTask->dynamicPriority) {
            nextTask->dynamicPriority--;
            nextTask = tryTask;
        }
        else {
            tryTask->dynamicPriority--;
        }

        tryTask = tryTask->next;
    }

    nextTask->dynamicPriority = nextTask->staticPriority;
    return nextTask;
}

void task_sleep(int t)
{
    currentTask->wakeTime = totalExecutionTime + t;

    enter_cs();
    queue_t *aux = queue_remove((queue_t **) &tasks, (queue_t *) currentTask);
    queue_append((queue_t **) &sleepingTasks, aux);
    leave_cs();

    if (nextWake == -1 || nextWake > currentTask->wakeTime) nextWake = currentTask->wakeTime;

    task_yield();
}

void awake_tasks() {
    nextWake = -1;

    task_t *tryWake = sleepingTasks;
    while (tryWake->next != sleepingTasks) {
        task_t *aux = tryWake->next;
        if (totalExecutionTime >= tryWake->wakeTime) {
            tryWake = (task_t *) queue_remove((queue_t **) &sleepingTasks, (queue_t *) tryWake);
            queue_append((queue_t **) &tasks, (queue_t *) tryWake);
        }
        else if (nextWake == -1 || nextWake > tryWake->wakeTime) {
            nextWake = tryWake->wakeTime;
        }

        tryWake = aux;
    }

    if (totalExecutionTime >= tryWake->wakeTime) {
        tryWake = (task_t *) queue_remove((queue_t **) &sleepingTasks, (queue_t *) tryWake);
        queue_append((queue_t **) &tasks, (queue_t *) tryWake);
    }
    else if (nextWake == -1 || nextWake > tryWake->wakeTime) {
        nextWake = tryWake->wakeTime;
    }
}

void dispatcher()
{
    dispatchControl = true;

    task_t *next;
    while (queue_size((queue_t *) tasks) > 0 || nextWake != -1)
    {
        if (nextWake != -1 && nextWake <= totalExecutionTime)
            awake_tasks();

        next = scheduler();

        if (next != NULL) {
            task_switch(next);
        }
    }
    task_exit(0);
}

void task_yield()
{
    task_switch(&dispatch);
}

void time_interruption() {
    totalExecutionTime += 1;

    currentTask->executionTime +=1;
    if (!lock && currentTask->id) {
        currentTask->quantum--;
        if (!currentTask->quantum) {
            currentTask->quantum = QUANTUM;
            task_yield();
        }
    }
}

void timer_init() {
    action.sa_handler = time_interruption ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0 ;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }
    
}

unsigned int systime () {
    return (totalExecutionTime);
}

void task_setprio (task_t *task, int prio) {
    if (task == NULL) {
        currentTask->staticPriority = prio;
        currentTask->dynamicPriority = prio;
    }
    else {
        task->staticPriority = prio;
        task->dynamicPriority = prio;
    }
}

int task_getprio (task_t *task) {
    if (task == NULL) return currentTask->staticPriority;
    else return task->staticPriority;
}

int task_join (task_t *task) {
   if (task->running) {
       enter_cs();
       queue_t *aux = queue_remove((queue_t **) &tasks, (queue_t *) currentTask); // remove calling task from the queue of ready tasks
       queue_append((queue_t **) &task->waitingTasks, aux); // add the calling task to the queue of suspended tasks of the joining task
       leave_cs();

       task_yield();
   }

    return (task->exitCode); // return the joined task id to the calling task
}

int sem_create (semaphore_t *s, int value) {
    if (s != NULL) {
        s->tasks = NULL;
        s->counter = value;
        s->destroyed = false;
        return (0);
    }

    return (-1);
}

int sem_down (semaphore_t *s) {
    enter_cs();
    if (s && !s->destroyed) {
        s->counter--;
        if (s->counter < 0) {
            queue_t *aux = queue_remove((queue_t **) &tasks, (queue_t *) currentTask);
            queue_append((queue_t **) &s->tasks, aux);

            leave_cs();
            task_yield();

            if (s->destroyed) return -1;
        }
        else leave_cs();

        return (0);
    }
    leave_cs();

    return (-1);
}

int sem_up (semaphore_t *s) {
    enter_cs();
    if (s->destroyed || s==NULL) {
        leave_cs();
        return -1;
    }

    s->counter++;
    if (s->counter <= 0) {
        queue_t *aux = queue_remove((queue_t **) &s->tasks, (queue_t *) s->tasks); // remove the first task of the semaphore queue
        queue_append((queue_t **) &tasks, aux); // return the removed task from the semaphores to the queue of ready tasks
    }
    leave_cs();

    return (0);
}

int sem_destroy (semaphore_t *s) {
    enter_cs();
    if (s && !s->destroyed) {
        queue_t *aux;
        while (s->tasks != NULL) {
            aux = queue_remove((queue_t **) &s->tasks, (queue_t *) s->tasks);
            queue_append((queue_t **) &tasks, aux);
        }

        s->destroyed = true;
        s = NULL;

        leave_cs();
        return (0);
    }

    leave_cs();
    return (-1);
}

int mqueue_create (mqueue_t *queue, int max, int size) {
    if (queue == NULL || max <= 0 || size <= 0) return -1;
    if (!( queue->msgs = (void **) malloc(sizeof(void *)*max*size) )) return -1;

    queue->max = max;
    queue->msgSize = size;
    queue->length = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->destroyed = false;

    sem_create(&queue->buffer, 1);
    sem_create(&queue->vaga, max);
    sem_create(&queue->item, 0);

    return 0;
}

int mqueue_send (mqueue_t *queue, void *msg) {
    if (queue && !queue->destroyed) {
        if (sem_down(&queue->vaga) == -1 || sem_down(&queue->buffer) == -1) return -1;

        memcpy(&(queue->msgs[queue->tail]), msg, queue->msgSize);
        queue->tail = (queue->tail + 1) % queue->max;
        queue->length++;

        sem_up(&queue->buffer);
        sem_up(&queue->item);

        return (0);
    }

    return (-1);
}

int mqueue_recv (mqueue_t *queue, void *msg) {
    if (queue && !queue->destroyed) {
        if (sem_down(&queue->item) == -1 || sem_down(&queue->buffer) == -1) return -1;

        memcpy(msg, &(queue->msgs[queue->head]), queue->msgSize);
        queue->head = (queue->head + 1) % queue->max;
        queue->length--;

        sem_up(&queue->buffer);
        sem_up(&queue->vaga);

        return (0);
    }

    return (-1);
}

int mqueue_destroy (mqueue_t *queue) {
    if (queue && !queue->destroyed) {
        sem_down(&queue->buffer);

        free(queue->msgs);
        queue->destroyed = true;

        sem_destroy(&queue->item);
        sem_destroy(&queue->vaga);
        sem_destroy(&queue->buffer);

        return (0);
    }

    return (-1);
}

int mqueue_msgs (mqueue_t *queue) {
    if (queue) return (queue->length);

    return (-1);    
}

void ppos_init()
{
    setvbuf(stdout, 0, _IONBF, 0);

    tasks = NULL;
    task_create(&dispatch, (void *)dispatcher, ""); // dispatcher will be identified as the task number 0

    timer_init();
    
    task_create(&mainTask, (void *)ppos_init, ""); // main will be identified as the task number 1
    currentTask = &mainTask;
}