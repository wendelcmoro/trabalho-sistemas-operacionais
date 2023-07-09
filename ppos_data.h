// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas
#include <stdbool.h>

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;		// ponteiros para usar em filas
   int id ;				// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   int staticPriority;
   int dynamicPriority;
   int quantum;
   long executionTime;
   int activations;
   bool running;        // Denotes if task hasn't ended
   int exitCode;
   long wakeTime;
   struct task_t *waitingTasks;
   // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct semaphore_t
{
  long counter;
  bool destroyed;
  struct task_t *tasks;
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct mqueue_t
{
  int max;
  int msgSize;
  int length;
  int head;
  int tail;
  bool destroyed;
  void **msgs; // array of msgs
  struct semaphore_t buffer;
  struct semaphore_t vaga;
  struct semaphore_t item;
} mqueue_t ;

#endif

