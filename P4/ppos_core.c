// GRR20190363 Luan Machado Bernardt

#include "ppos.h"

#include <stdlib.h>
#include <stdio.h>

#define STACKSIZE 64*1024

#define TA_ALFA -1   // Fator de aging
#define MAX_PRIO -20 // Maior prioridade
#define MIN_PRIO 20  // Menor prioridade

task_t *currentTask, *prevTask, *taskQueue,  mainTask, dispatcherTask;
int g_taskId = 0, g_userTasks = 0;

static void dispatcher ();

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {

    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);

    mainTask.next = mainTask.prev = NULL;
    mainTask.id = g_taskId;
    mainTask.status = 'R';
    mainTask.pDinamica = mainTask.pEstatica = 0;
    
    currentTask = &mainTask;

    if (task_create(&dispatcherTask, dispatcher, NULL) == -1) {
        perror("### Erro ao criar a tarefa dispatcher ###");
        exit(1);
    };
    
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro
int task_create (task_t *task, void (*start_func)(void *), void *arg) {

    getcontext(&task->context);
     
    char *stack = malloc (STACKSIZE); // Inicializacao do tipo context
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
    task->id = ++g_taskId;
    task->status = 'R';
    task->pEstatica = task->pDinamica = 0;

    if (task != &dispatcherTask) {// Se nao for a tarefa dispatcher 
        if (queue_append((queue_t **) &taskQueue, (queue_t *) task) == 1) // Coloca-se na fila de tarefas
            fprintf(stderr,"### Erro ao inserir a tarefa %d em taskQueue ###\n", g_taskId);
        else
            g_userTasks++;
    }
    #ifdef DEBUG
        printf ("task_create: criou tarefa %d\n", task->id);
        printf ("numero de tarefas na fila %d\n", queue_size((queue_t *) taskQueue));
    #endif

    return g_taskId;
}

// Alterna a execução para a tarefa indicada
int task_switch (task_t *task) {

    if (!task)
        return -1;

    prevTask = currentTask; // Tarefa anterior recebe a tarefa atual
    currentTask = task;     // Tarefa atual recebe task
    
     #ifdef DEBUG
        printf ("task_switch: trocando contexto %d(%c) -> %d(%c)\n", prevTask->id, prevTask->status, 
        currentTask->id, currentTask->status);
    #endif
    
    swapcontext(&prevTask->context,&task->context);

    return 0;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode) {

    currentTask->status = 'T'; // Muda o status da tarefa atual para terminada
    
    #ifdef DEBUG
        printf ("task_exit: tarefa %d sendo encerrada\n", currentTask->id);
    #endif
    
    if (currentTask != &dispatcherTask) // Se a tarefa terminada nao for a dispatcher
        task_switch(&dispatcherTask);   // Alterna a execucao para dispatcher
    else
        task_switch(&mainTask); // Alterna a execucao para main
}

// Retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {

    return currentTask->id;
}

// Libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield () {

    task_switch(&dispatcherTask);
}

// Define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio) {

    if (!task)
        task = currentTask;
    if (prio > MIN_PRIO) // Se prio for menor que o limite inferior
        task->pEstatica = task->pDinamica = MIN_PRIO;
    else if (prio < MAX_PRIO) // Se prio for maior que o limite superior
        task->pEstatica = task->pDinamica = MAX_PRIO;
    else 
        task->pEstatica = task->pDinamica = prio;
}

// Retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task) {

    if (task)
        return task->pEstatica;
    return currentTask->pEstatica;
}

// Funcao que retorna a proxima tarefa a ser executada
static task_t *scheduler () {

    task_t *task = taskQueue; // Inicia na primeira tarefa pronta
    task_t *highestPrio = task;

    if (!task)
        return NULL;

    while ((task = task->next) != taskQueue) { // Enquanto task nao voltar a primeira da fila
        if (highestPrio->pDinamica > task->pDinamica) {
            highestPrio->pDinamica += TA_ALFA; // Task aging
            highestPrio = task;
        }
        else
            task->pDinamica += TA_ALFA; // Task aging
    }

    highestPrio->pDinamica = highestPrio->pEstatica;
    
    return highestPrio; 
}

// Funcao executada na dispatcherTask, responsavel por alternar a execucao
// entre as tarefas prontas
static void dispatcher () {

    task_t *nextTask;

    while (g_userTasks > 0) { // Enquanto existirem tarefas a serem executadas
            
        if ((nextTask = scheduler ())) { // Se a proxima tarefa existir
        
            task_switch(nextTask);

            if (prevTask->status == 'T') { // Se terminada 'T' a tarefa e removida da fila
                    
                queue_remove((queue_t **) &taskQueue, (queue_t *) prevTask);
                free(prevTask->context.uc_stack.ss_sp); // Desaloca a pilha
                g_userTasks--;
                #ifdef DEBUG
                    printf ("numero de tarefas na fila %d\n", queue_size((queue_t *) taskQueue));
                #endif
            }
        }
    }
    
    task_exit(0);
}