// GRR20190363 Luan Machado Bernardt

#include "ppos.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

#define STACKSIZE 64*1024

#define TA_ALFA -1   // Fator de aging
#define MAX_PRIO -20 // Maior prioridade
#define MIN_PRIO 20  // Menor prioridade
#define QUANTUM 1   // Ticks por quantum

task_t *currentTask, *prevTask, *taskQueue,  mainTask, dispatcherTask;
int task_Id = 0, userTasks = 0, temporizador;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização to timer
struct itimerval timer ;

static void dispatcher ();
static void set_timer ();
static void tratador ();

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {

    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);

    getcontext(&mainTask.context);

    mainTask.next = mainTask.prev = NULL;
    mainTask.id = task_Id;
    mainTask.status = 'E';
    mainTask.pDinamica = mainTask.pEstatica = 0;
    
    currentTask = &mainTask;

    task_create(&dispatcherTask, dispatcher, NULL);
    temporizador = QUANTUM;

    set_timer();
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
    task->pEstatica = task->pDinamica = 0;

    if (task != &dispatcherTask) { // Se nao for a tarefa dispatcher 
        queue_append((queue_t **) &taskQueue, (queue_t *) task); // Coloca-se na fila de tarefas
        userTasks++;
    }
    #ifdef DEBUG
        printf ("task_create: criou tarefa %d\n", task->id);
        printf ("numero de tarefas na fila %d\n", queue_size((queue_t *) taskQueue));
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

    temporizador = QUANTUM;
    swapcontext(&prevTask->context,&task->context);

    return 0;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode) {

    currentTask->status = 'T';
    
    #ifdef DEBUG
        printf ("task_exit: tarefa %d sendo encerrada\n", currentTask->id);
    #endif
    
    if (currentTask != &dispatcherTask) // Se a tarefa terminada nao for a dispatcher
        task_switch(&dispatcherTask); // Alterna a execucao para dispatcher
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

    while (1) {
        while (userTasks > 0) { // Enquanto existirem tarefas a serem executadas
                
            if ((nextTask = scheduler ())) { // Se a proxima tarefa existir
            
                task_switch(nextTask);

                if (prevTask->status == 'T') { // Se terminada 'T' a tarefa e removida da fila
                        
                    queue_remove((queue_t **) &taskQueue, (queue_t *) prevTask);
                    free(prevTask->context.uc_stack.ss_sp);
                    userTasks--;
                    #ifdef DEBUG
                        printf ("numero de tarefas na fila %d\n", queue_size((queue_t *) taskQueue));
                    #endif
                }
            }
        }
        task_exit(0);
    }
}

static void set_timer () {

    // registra a ação para o sinal de timer SIGALRM
    action.sa_handler = tratador ;
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

// Funcao que trata os ticks do timer
static void tratador () {

    if (currentTask == &dispatcherTask)
        return;
    if (temporizador == 0)
        task_yield();
    temporizador--;
}