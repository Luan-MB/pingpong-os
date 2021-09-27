// GRR20190363 Luan Machado Bernardt

#include "ppos.h"
#include <stdlib.h>
#include <stdio.h>

#define STACKSIZE 64*1024

task_t *currentTask, *prevTask, mainTask;
int task_Id = 0;

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init () {

    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);

    mainTask.id = task_Id;
    mainTask.next = mainTask.prev = NULL; // Atribuicao necessaria para colocar em uma fila mais a frente
    
    currentTask = &mainTask;
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

    makecontext (&task->context, (void *)(*start_func), 1, arg); // Ajusta a funcao a ser executada em task
    task->next = task->prev = NULL; // Atribuicao necessaria para colocar em uma fila mais a frente
    task->id = ++task_Id;

    #ifdef DEBUG
        printf ("task_create: criou tarefa %d\n", task->id);
    #endif

    return task_Id;
}

// Alterna a execução para a tarefa indicada
int task_switch (task_t *task) {

    if (!task)
        return -1;
    
    #ifdef DEBUG
        printf ("task_switch: trocando contexto %d -> %d\n", currentTask->id, task->id);
    #endif
    
    prevTask = currentTask;
    currentTask = task;

    swapcontext(&prevTask->context,&task->context); // Salva o contexto atual em prevTask e carrega o contido em currentTask

    return 0;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode) {

    #ifdef DEBUG
        printf ("task_exit: tarefa %d sendo encerrada\n", currentTask->id);
    #endif
    task_switch(&mainTask); // Volta para o contexto mainTask
}

// Retorna o identificador da tarefa corrente (main deve ser 0)
int task_id () {

    return currentTask->id;
}
