// GRR20190363 Luan Machado Bernardt

#include "ppos.h"
#include <stdlib.h>
#include <stdio.h>

#define STACKSIZE 64*1024

task_t *currentTask, *prevTask, mainTask;
int task_Id = 0;

void ppos_init () {

    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);

    mainTask.id = task_Id;
    
    currentTask = &mainTask;
}

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
    task->id = ++task_Id;

    #ifdef DEBUG
        printf ("task_create: criou tarefa %d\n", task->id);
    #endif

    return task_Id;
}

int task_switch (task_t *task) {

    if (!task)
        return -1;
    
    #ifdef DEBUG
        printf ("task_switch: trocando contexto %d -> %d\n", currentTask->id, task->id);
    #endif
    
    prevTask = currentTask;
    currentTask = task;

    swapcontext(&prevTask->context,&task->context);

    return 0;
}

void task_exit (int exitCode) {

    #ifdef DEBUG
        printf ("task_exit: tarefa %d sendo encerrada\n", currentTask->id);
    #endif
    task_switch(&mainTask);
}

int task_id () {

    return currentTask->id;
}
