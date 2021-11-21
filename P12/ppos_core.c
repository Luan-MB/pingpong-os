// GRR20190363 Luan Machado Bernardt

#include "ppos.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#define STACKSIZE 64*1024

#define TA_ALFA -1   // Fator de aging
#define MAX_PRIO -20 // Maior prioridade
#define MIN_PRIO 20  // Menor prioridade
#define QUANTUM 20   // Ticks por quantum

task_t *currentTask, *prevTask, *taskQueue, *sleepingQueue;
task_t mainTask, dispatcherTask;
int g_taskId = 0, g_userTasks = 0, g_taskTime;
unsigned int g_clock = 0, g_taskActivTime = 0;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização to timer
struct itimerval timer ;

static void dispatcher ();  // Funcao dispatcher
static void set_timer ();   // Funcao que inicia o timer
static void tratador ();    // Funcao que trata os ticks do timer
unsigned int systime ();    // Funcao que retorna o numeros de ticks desde o inicio de timer
static void wakeup_tasks(); // Funcao que devolve a fila de tarefas as prontas para acordar

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {

    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);

    getcontext(&mainTask.context);

    // Inicializacoes dda tarefa main
    mainTask.next = mainTask.prev = NULL;
    mainTask.id = g_taskId++;
    mainTask.status = 'R';
    mainTask.pDinamica = mainTask.pEstatica = 0;
    mainTask.taskType = 1;
    mainTask.eTime = systime ();
    mainTask.pTime = 0;
    mainTask.joinQueue = NULL;
    
    // Insere main na fila de tarefas prontas
    if (queue_append((queue_t **) &taskQueue, (queue_t *) &mainTask) == 1) { // Se der erro aborta-se o programa
        fprintf(stderr,"### Erro ao inserir a tarefa %d em taskQueue ###\n", mainTask.id);
        exit(1);
    }
    else
        g_userTasks++;
    currentTask = &mainTask;

    // Cria a tarefa dispatcher
    task_create(&dispatcherTask, dispatcher, NULL);
    dispatcherTask.taskType = SYSTEM;

    set_timer(); // Inicia o timer

    task_yield(); // Entrega o processador para dispatcher
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro
int task_create (task_t *task, void (*start_func)(void *), void *arg) {

    getcontext(&task->context);

    char *stack = malloc (STACKSIZE);
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
    
    // Atribuicoes iniciais
    task->next = task->prev = NULL;         
    task->id = g_taskId++;
    task->status = 'R';
    task->pEstatica = task->pDinamica = 0;
    task->taskType = USER;
    task->eTime = systime ();
    task->pTime = 0;
    task->activations = 0;
    task->joinQueue = NULL;

    if (task != &dispatcherTask) { // Se nao for a tarefa dispatcher 
        if (queue_append((queue_t **) &taskQueue, (queue_t *) task) == 1) // Coloca task na fila de tarefas
            fprintf(stderr,"### Erro ao inserir a tarefa %d em taskQueue ###\n", task->id);
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
    
    currentTask = task; // Tarefa atual recebe task

    if (prevTask->status != 'T') // Se prevTask nao tiver terminado ('T')
        prevTask->pTime += (systime() - g_taskActivTime); // Calcula o tempo de processador da tarefa substituida 
    
    g_taskActivTime = systime (); // Salva o tempo de ativacao da tarefa

    #ifdef DEBUG
        printf ("task_switch: trocando contexto %d(%c) -> %d(%c)\n", prevTask->id, prevTask->status, 
        currentTask->id, currentTask->status);
    #endif
    
    swapcontext(&prevTask->context,&task->context);

    return 0;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode) {

    #ifdef DEBUG
        printf ("task_exit: tarefa %d sendo encerrada\n", currentTask->id);
    #endif
    
    currentTask->status = 'T'; // Status da tarefa terminada
    currentTask->pTime += (systime() - g_taskActivTime); // Calcula o tempo de processador da tarefa encerrada
    currentTask->exitCode = exitCode;
    currentTask->eTime = (systime () - currentTask->eTime); // Calcula o tempo de execucao da tarefa
    
    printf("task %d exit: Execution time: %d ms, processor time: %d ms, activations: %d\n",currentTask->id,currentTask->eTime,currentTask->pTime,currentTask->activations);

    queue_t *aux;
    while ((aux = currentTask->joinQueue)) { // Enquanto houver tarefas na fila de espera
        queue_remove((queue_t **) &currentTask->joinQueue, aux);
        queue_append((queue_t **) &taskQueue, aux);
    }
    
    if (currentTask != &dispatcherTask) // Se a tarefa terminada nao for a dispatcher
        task_switch(&dispatcherTask); // Alterna a execucao para dispatcher
    else
        exit(0); // Se for o dispatcher encerra o programa
}

// Retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {

    return currentTask->id;
}

// Libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield () {

    #ifdef DEBUG
        printf("task_yield: task %d yields\n", currentTask->id);
    #endif

    dispatcherTask.activations++;
    task_switch(&dispatcherTask);
}

// Define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio) {

    if (!task)
        task = currentTask;

    #ifdef DEBUG
        printf("task_setprio: trocando a prioridade da tarefa %d de %d -> %d\n", task->id, task->pEstatica, prio);
    #endif

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

// A tarefa corrente aguarda o encerramento de outra task
int task_join (task_t *task) {

    if (!task || task->status == 'T') // Se task nao existir ou tiver terminado
        return -1;
    queue_remove((queue_t **) &taskQueue, (queue_t *) currentTask); // Retira a tarefa da fila de prontas
    queue_append((queue_t **) &task->joinQueue, (queue_t *) currentTask); // Insere a tarefa na fila de espera de task
    task_yield();
    return task->exitCode;
}

// Suspende a tarefa corrente por t milissegundos
void task_sleep (int t) {

    currentTask->wakeupTime = systime () + t; // Calcula o tempo em qua a tarefa deve acordar
    currentTask->status = 'S';                // troca o status para suspensa
    queue_remove((queue_t **) &taskQueue, (queue_t *) currentTask);     // Retira a tarefa da fila de prontas
    queue_append((queue_t **) &sleepingQueue, (queue_t *) currentTask); // Coloca na fila de tarefas suspensas

    #ifdef DEBUG
        printf("task_sleep: tarefa %d a dormir por %d ms zzzzz...\n", systime(), currentTask->id, t);
    #endif
    
    task_yield();
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
        
        wakeup_tasks();

        if ((nextTask = scheduler ())) { // Se a proxima tarefa existir
            
            nextTask->activations++;
            g_taskTime = QUANTUM;
    
            task_switch(nextTask);

            if (prevTask->status == 'T') { // Se terminada 'T' a tarefa e removida da fila
                    
                queue_remove((queue_t **) &taskQueue, (queue_t *) prevTask);
                free(prevTask->context.uc_stack.ss_sp);
                g_userTasks--;
                #ifdef DEBUG
                    printf ("numero de tarefas na fila %d\n", queue_size((queue_t *) taskQueue));
                #endif
            }
        }
        
    }
    
    task_exit(1);
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

    // ajusta valores do timer
    timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0 ;         // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;      // disparos subsequentes, em segundos

    // arma o timer ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
      perror ("Erro em setitimer: ") ;
      exit (1) ;
    }
}

// Funcao que trata os ticks do timer
static void tratador () {

    g_clock++;
    if (currentTask->taskType == SYSTEM) // Tarefas de sistema nao devem ser preemptadas
        return;
    g_taskTime--;
    if (g_taskTime == 0)
        task_yield();
}

unsigned int systime () {

    return g_clock;
}

// Funcao que devolve a fila de tarefas as prontas para acordar
static void wakeup_tasks() {

    int sleepQueueSize = queue_size ((queue_t *) sleepingQueue);
        
    if (sleepQueueSize > 0) { // Se existirem tarefas dormindo

        task_t *aux = sleepingQueue;
        task_t *wakingUpTask;
        int systimeLock = systime();

        for (int i=0; i<sleepQueueSize; ++i) {
            
            if (aux->wakeupTime <= systimeLock) { // Se o tempo de acordar chegou
                aux = aux->next;
                wakingUpTask = aux->prev;
                queue_remove((queue_t **) &sleepingQueue, (queue_t *) wakingUpTask); // Retira da fila de suspensas
                queue_append((queue_t **) &taskQueue, (queue_t *) wakingUpTask);     // Coloca na fila de prontas
                wakingUpTask->status = 'R';            // Troca o status para pronta

                #ifdef DEBUG
                    printf("task_wakeup: task %d acordou aos %d ms, bom dia\n",wakingUpTask->id, systimeLock);
                #endif
    
            } else 
                aux = aux->next; 
        }
    }
}

static void enter_cs (int *lock)
{
    // atomic OR (Intel macro for GCC)
    while (__sync_fetch_and_or (lock, 1)) ;   // busy waiting
}
 
static void leave_cs (int *lock)
{
    (*lock) = 0 ;
}

// Cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value) {
    
    if (!s)
        return -1;

    s->value = value;
    s->sQueue = NULL;
    s->isValid = 1;
    s->semLock = 0;

    #ifdef DEBUG
        printf("sem_create: semaforo com valor %d criado\n",value);
    #endif

    return 0;
}

// Requisita o semáforo
int sem_down (semaphore_t *s) {

    if (!s || !s->isValid)
        return -1;

    #ifdef DEBUG
        printf("sem_down: tarefa %d solicitou sem_down\n",currentTask->id);
    #endif
    
    enter_cs(&s->semLock);
    s->value--;
    leave_cs(&s->semLock);

    if (s->value < 0) { // Coloca a tarefa atual na fila do semaforo
        queue_remove((queue_t **) &taskQueue, (queue_t *) currentTask);
        queue_append((queue_t **) &s->sQueue, (queue_t *) currentTask);
        
        #ifdef DEBUG
            printf("sem_down: tarefa %d colocada na espera no semaforo\n",currentTask->id);
        #endif

        task_yield();
    }
    
    return 0;
}

// Libera o semáforo
int sem_up (semaphore_t *s) {

    if (!s || !s->isValid)
        return -1;
    #ifdef DEBUG
        printf("sem_up: tarefa %d solicitou sem_up\n",currentTask->id);
    #endif

    enter_cs(&s->semLock);
    s->value++;
    leave_cs(&s->semLock);

    if (s->value >= 0) { // Devolve a primeira tarefa na fila do semaforo para taskQueue
        task_t *firstElem;

        if ((firstElem = s->sQueue)) {
            queue_remove((queue_t **) &s->sQueue, (queue_t *) firstElem);
            queue_append((queue_t **) &taskQueue, (queue_t *) firstElem);
            #ifdef DEBUG
                printf("sem_up: tarefa %d retirada do semaforo\n",firstElem->id);
            #endif
        }
    }
    
    return 0;
}

// Destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s) {

    if (!s || !s->isValid)
        return -1;
    
    task_t *aux;

    while ((aux = s->sQueue)) { // Devolve todas as tarefas da filado semaforo para taskQueue
        queue_remove((queue_t **) &s->sQueue, (queue_t*) aux);
        queue_append((queue_t **) &taskQueue, (queue_t*) aux);
    }

    #ifdef DEBUG
        printf("sem_destroy: semaforo destruido\n");
    #endif
    s->isValid = 0; // Semaforo passa a ser invalido
    return 0;
}

// filas de mensagens

// Cria uma fila para até max mensagens de size bytes cada
int mqueue_create (mqueue_t *queue, int max, int size) {

    if (!queue)
        return -1;
    
    queue->msgBuffer = malloc(max * size);
    queue->bufferSize = max;
    queue->msgSize = size;
    queue->isValid = 1;
    queue->firstElem = queue->lastElem = 0;

    if (sem_create(&queue->sendSem, max))
        return -1;
    if (sem_create(&queue->recvSem, 0))
        return -1;
    if (sem_create(&queue->bufferSem, 1))
        return -1;
    
    return 0;
}

// Envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg) {

    if (!queue || !msg || !queue->isValid)
        return -1;
    
    sem_down(&queue->sendSem);

    sem_down(&queue->bufferSem);
    // Calcula a "ultima posicao" do buffer circular e copia a mensagem
    memcpy(queue->msgBuffer + (queue->lastElem * queue->msgSize), msg, queue->msgSize);
    queue->lastElem = (queue->lastElem + 1) % queue->bufferSize;
    sem_up(&queue->bufferSem);

    sem_up(&queue->recvSem);
    return 0;
}

// Recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg) {

    if (!queue || !queue->isValid)
        return -1;

    sem_down(&queue->recvSem);

    sem_down(&queue->bufferSem);
    // Calcula a "primeira posicao" do buffer circular e recebe a mensagem
    memcpy(msg, queue->msgBuffer + (queue->firstElem * queue->msgSize), queue->msgSize);
    queue->firstElem = (queue->firstElem + 1) % queue->bufferSize;
    sem_up(&queue->bufferSem);

    sem_up(&queue->sendSem);
    return 0;
}

// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue) {

    if (!queue || !queue->isValid)
        return -1;
    
    if (sem_destroy(&queue->bufferSem))
        return -1;
    if (sem_destroy(&queue->recvSem))
        return -1;
    if (sem_destroy(&queue->sendSem))
        return -1;

    queue->isValid = 0;
    free(queue->msgBuffer);
    queue->msgBuffer = NULL;

    return 0;
}

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue) {

    if (!queue || !queue->isValid)
        return -1;
    
    return queue->recvSem.value;
}