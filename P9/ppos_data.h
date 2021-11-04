// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		  // biblioteca de filas genéricas

typedef enum {SYSTEM, USER} type;

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
    struct task_t *prev, *next;		  // ponteiros para usar em filas
    int id;				                  // identificador da tarefa
    ucontext_t context;			        // contexto armazenado da tarefa
    char status;                    // R -> ready, T -> terminada, W -> esperando, S -> dormindo
    int pDinamica, pEstatica;       // prioridades dinamica e estatica da tarefa
    type taskType;                  // tipos de tarefa SYSTEM -> Sistema, USER -> Usuario
    unsigned int eTime, pTime;      // tempos de execucao e processamento
    unsigned int activations;       // numero de ativacoes da tarefa
    queue_t *joinQueue;             // aponta para a fila de tarefas esperando o termino
    int exitCode;                   // codigo de saida da tarefa
    unsigned int wakeupTime;         // tempo em que a tarefa acoradara
   // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
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
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif

