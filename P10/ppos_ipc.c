// GRR20190363 Luan Machado Bernardt

#include "ppos.h"
#include "queue.h"
#include "ppos_global_vars.h"

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
    s->semLock = 0;

    #ifdef DEBUG
        printf("sem_create: semaforo com valor %d criado\n",value);
    #endif

    return 0;
}

// Requisita o semáforo
int sem_down (semaphore_t *s) {

    if (!s)
        return -1;

    #ifdef DEBUG
        printf("sem_down: tarefa %d solicitou sem_down\n",currentTask->id);
    #endif
    
    enter_cs(&s->semLock);
    s->value--;
    leave_cs(&s->semLock);

    if (s->value < 0) {
        queue_remove((queue_t **) &taskQueue, (queue_t *) currentTask);
        queue_append(&s->sQueue, (queue_t *) currentTask);
        task_yield();
    }
    
    return 0;
}

// Libera o semáforo
int sem_up (semaphore_t *s) {

    if (!s)
        return -1;
    #ifdef DEBUG
        printf("sem_up: tarefa %d solicitou sem_up\n",currentTask->id);
    #endif

    enter_cs(&s->semLock);
    s->value++;
    
    
    if (s->sQueue) {

        queue_t *firstElem = s->sQueue;

        queue_remove(&s->sQueue, firstElem);
        queue_append((queue_t **) &taskQueue, firstElem);
    }
    leave_cs(&s->semLock);
    return 0;
}

// Destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s) {

    if (!s)
        return -1;
    
    int semQueueSize;

    if ((semQueueSize = queue_size(s->sQueue)) > 0) {

        queue_t *aux = s->sQueue;

        for (int i=0; i<semQueueSize; ++i) {
            queue_remove(&s->sQueue, aux);
            queue_append((queue_t **) &taskQueue, aux);
        }
    }

    #ifdef DEBUG
        printf("sem_destroy: semaforo destruido\n");
    #endif
    s = NULL;
    return 0;
}