// GRR20190363 Luan Machado Bernardt

#include "queue.h"
#include <stdio.h>

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila
int queue_size (queue_t *queue) {

    if (!queue) // Se a fila estiver vazia
        return 0;
    
    queue_t *aux = queue;
    int size = 1;

    while ((aux = aux->next) != queue) // Se a fila possuir ao menos um elemento
        size++;

    return size;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir
void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {

    printf("%s: [",name);
    
    if (queue) { // Se a fila nao estiver vazia
        
        queue_t *aux = queue;
        
        while (aux->next != queue) { // Percorre a fila ate o penultimo elemento
            print_elem(aux);
            printf(" ");
            aux = aux->next;
        }
        print_elem(aux); // Imprime o ultimo
        
    }

    printf("]\n");
}

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_append (queue_t **queue, queue_t *elem) {

    if (!queue) {
        perror("### A fila nao existe ###");
        return 1;
    }
    
    if (!elem) {
        perror("### O elemento nao existe ###");
        return 1;
    }
    
    if (elem->next || elem->prev) {
        perror("### O elemento esta em uma fila ###");
        return 1;
    }
    
    if (!(*queue)) { // Se a fila estiver vazia
        *queue = elem;
        (*queue)->next = (*queue)->prev = *queue;
        return 0;
    }
        
    queue_t *aux = *queue;

    while(aux->next != *queue) // Se a fila possuir ao menos um elemento
        aux = aux->next;

    (*queue)->prev = elem;
    elem->prev = aux;
    elem->next = *queue;
    aux->next = elem;

    return 0;
}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: 0 se sucesso, <0 se ocorreu algum erro
int queue_remove (queue_t **queue, queue_t *elem) {

    if (!queue) {
        perror("### A fila nao existe ###");
        return 1;
    }

    if (!(*queue)) {
        perror("### A fila esta vazia ###");
        return 1;
    }
    
    if (!elem) {
        perror("### O elemento nao existe ###");
        return 1;
    }
    
    if (!(elem->prev) && !(elem->next)) {
        perror("### O elemento nao pertence a nenhuma fila ###");
        return 1;
    }

    if (elem == *queue) { // Caso de elem ser o primeiro elemento da lista
        
        if (*queue == (*queue)->next) { // Caso de elem ser o unico elemento da lista
            *queue = NULL;
        }
        else { // Caso de elem nao ser o unico elemento da lista
            (*queue)->prev->next = (*queue)->next;
            (*queue)->next->prev = (*queue)->prev;
            *queue = (*queue)->next;
        }
        elem->next = elem->prev = NULL;
        return 0;
    }

    queue_t *aux = *queue;

    while ((aux = aux->next) != *queue) { // Percorre a fila em busca de elem
        if (aux == elem) { // Se elem foi encontrado
            aux->prev->next = aux->next;
            aux->next->prev = aux->prev;
            elem->next = elem->prev = NULL;
            return 0;
        }
    }

    perror("### O elemento nao esta na fila ###");
    return 1;
}