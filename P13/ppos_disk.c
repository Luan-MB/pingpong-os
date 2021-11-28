#include "ppos_disk.h"
#include "disk.h"
#include "ppos.h"
#include "globalvars.h"

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

task_t diskManagerTask;
task_t *currentTask;
task_t *taskQueue, *suspendedQueue;
disk_t disk;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

void disk_signal_handler();

static request_t *create_request(int block, void *buffer, request_type reqType) {

    request_t *newRequest = malloc (sizeof (request_t));
    if (!newRequest) return 0;

    newRequest->next = newRequest->prev = NULL;
    newRequest->req_task = currentTask;
	newRequest->req_type = reqType;
	newRequest->req_block = block;
	newRequest->req_buffer = buffer;

    return newRequest;
}

static void suspend_task (task_t *task) {

    queue_remove((queue_t **) &taskQueue, (queue_t *) task);
    queue_append((queue_t **) &suspendedQueue, (queue_t *) task);
    task->status = 'S';
}

static void enable_task (task_t *task) {

    queue_remove((queue_t **) &suspendedQueue, (queue_t *) task);
    queue_append((queue_t **) &taskQueue, (queue_t *) task);
    task->status = 'R';
}

void disk_manager() {
    
    while (1) {
        
        // obtém o semáforo de acesso ao disco
        sem_down(&disk.disk_sem);
        // se foi acordado devido a um sinal do disco
        if (disk.disk_signal) {
            task_t *servedTask = disk.disk_request->req_task;
            enable_task(servedTask);
            queue_remove((queue_t **) &disk.disk_queue, (queue_t *) disk.disk_request);
            free(disk.disk_request);

            disk.disk_signal = 0;
        }
        
        int disk_status = disk_cmd (DISK_CMD_STATUS, 0, 0);
        // se o disco estiver livre e houver pedidos de E/S na fila
        if ((disk_status == 1) && disk.disk_queue) {
            disk.disk_request = disk.disk_queue;
            if (disk.disk_request->req_type == READ)
                disk_cmd (DISK_CMD_READ, disk.disk_request->req_block, disk.disk_request->req_buffer);
            else
                disk_cmd (DISK_CMD_WRITE, disk.disk_request->req_block, disk.disk_request->req_buffer);
        }

        sem_up(&disk.disk_sem);

        suspend_task(&diskManagerTask);

        task_yield ();
    }
}

int disk_mgr_init (int *numBlocks, int *blockSize) {

    if (disk_cmd(DISK_CMD_INIT, 0, 0))
        return -1;
    
    if ((*numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0)) < 0)
        return -1;
    if ((*blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0)) < 0)
        return -1;

    sem_create(&disk.disk_sem, 1);
    
    disk.disk_signal = 0;

    task_create(&diskManagerTask, disk_manager, NULL);
    
    queue_append((queue_t **) &suspendedQueue, (queue_t *) &diskManagerTask);
    diskManagerTask.status = 'S';

    // registra a ação para o sinal de SIGUSR1
    action.sa_handler = disk_signal_handler;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGUSR1, &action, 0) < 0)
    {
      perror ("Erro em sigaction: ") ;
      exit (1) ;
    }

    return 0;
}

int disk_block_read (int block, void *buffer) {
    // obtém o semáforo de acesso ao disco
    sem_down(&disk.disk_sem);
    // inclui o pedido na fila_disco
    
    request_t *newRequest = create_request(block, buffer, READ);
    if (!newRequest)
        return -1;

    queue_append((queue_t **) &disk.disk_queue, (queue_t *) newRequest);

    if (diskManagerTask.status == 'S')
        enable_task(&diskManagerTask);
 
    // libera semáforo de acesso ao disco
    sem_up(&disk.disk_sem);
    // suspende a tarefa corrente (retorna ao dispatcher)
    suspend_task(currentTask);

    task_yield();

    return 0;
}

int disk_block_write (int block, void *buffer) {
    // obtém o semáforo de acesso ao disco
    sem_down(&disk.disk_sem);
    // inclui o pedido na fila_disco
    
    request_t *newRequest = create_request(block, buffer, WRITE);
    if (!newRequest)
        return -1;

    queue_append((queue_t **) &disk.disk_queue, (queue_t *) newRequest);
    
    if (diskManagerTask.status == 'S')
        enable_task(&diskManagerTask);
 
    // libera semáforo de acesso ao disco
    sem_up(&disk.disk_sem);
    // suspende a tarefa corrente (retorna ao dispatcher)
    suspend_task(currentTask);
    task_yield();

    return 0;
}

void disk_signal_handler () {

    disk.disk_signal = 1;
    if (diskManagerTask.status == 'S')
        enable_task(&diskManagerTask);
    
}