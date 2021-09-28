// GRR20190363 Luan Machado Bernardt

#include "ppos.h"

#include <stdlib.h>
#include <stdio.h>

#define STACKSIZE 64*1024

task_t *currentTask, *prevTask, *readyQueue,  mainTask, dispatcherTask;
int task_Id = 0;
int readyTasks = -1; // Comeca com -1 pois dispatcher nao deve contar como tarefa a ser executada

static void dispatcher ();

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {

    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);

    mainTask.next = mainTask.prev = NULL;
    mainTask.id = task_Id;
    mainTask.status = 'E';
    
    currentTask = &mainTask;

    task_create(&dispatcherTask, dispatcher, NULL);
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro
int task_create (task_t *task, void (*start_func)(void *), void *arg) {

    char *stack;

    getcontext(&task->context);

    stack = malloc (STACKSIZE);
    if (stack) {
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    } else {
        perror ("Erro na criação da pilha: ");
        return -1;
    }

    makecontext (&task->context, (void *)(*start_func), 1, arg);
    
    task->next = task->prev = NULL;
    task->id = ++task_Id;
    task->status = 'R';

    queue_append((queue_t **) &readyQueue, (queue_t *) task);
    readyTasks++;
    
    #ifdef DEBUG
        printf ("task_create: criou tarefa %d\n", task->id);
        printf ("numero de tarefas na fila %d\n", queue_size((queue_t *) readyQueue));
        printf ("numero de tarefas prontas %d\n", readyTasks);
    #endif

    return task_Id;
}

// Alterna a execução para a tarefa indicada
int task_switch (task_t *task) {

    if (!task)
        return -1;

    prevTask = currentTask; // Tarefa anterior recebe a tarefa atual
    currentTask = task;     // Tarefa atual recebe task
    
    if (prevTask->status != 'T') // Se prevTask nao tiver terminado ('T')
        prevTask->status = 'R';  // Troca-se status para pronta ('R')
    currentTask->status = 'E';   // Troca-se o status de current task para executando ('E')

    #ifdef DEBUG
        printf ("task_switch: trocando contexto %d(%c) -> %d(%c)\n", prevTask->id, prevTask->status, 
        currentTask->id, currentTask->status);
    #endif

    swapcontext(&prevTask->context,&task->context);

    return 0;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode) {

    currentTask->status = 'T';
    
    #ifdef DEBUG
        printf ("task_exit: tarefa %d sendo encerrada\n", currentTask->id);
    #endif
    
    if (currentTask != &dispatcherTask)
        task_switch(&dispatcherTask);
    else
        task_switch(&mainTask);
}

// Retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {

    return currentTask->id;
}

// Libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield () {

    if (currentTask->id != 0) { // Coloca a tarefa que estava sendo executada no fim da fila
        queue_remove((queue_t **) &readyQueue, (queue_t *) currentTask);
        queue_append((queue_t **) &readyQueue, (queue_t *) currentTask);
    }
    
    task_switch(&dispatcherTask);
}

// Funcao que retorna a proxima tarefa a ser executada
static task_t *scheduler () {

    // Como nao ha prioridade e a ultima tarefa executada foi para o fim
    // a proxima a ser executada esta sempre na posicao next de readyQueue
    return readyQueue->next;
}

// Funcao executada na dispatcherTask, responsavel por alternar a execucao
// entre as tarefas prontas
static void dispatcher () {

    task_t *nextTask;

    while (readyTasks > 0) {
        
        // Recebe a proxima tarefa a ser executada
        nextTask = scheduler();
    
        task_switch(nextTask);

        if (prevTask->status == 'T') { // Se terminada 'T' a tarefa e removida da fila
            
            queue_remove((queue_t **) &readyQueue, (queue_t *) prevTask);
            free(prevTask->context.uc_stack.ss_sp);
            readyTasks--;
            #ifdef DEBUG
                printf ("numero de tarefas na fila %d\n", queue_size((queue_t *) readyQueue));
                printf ("numero de tarefas prontas %d\n", readyTasks);
            #endif
        }
    }
    
    task_exit(0);
}