// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Teste de semáforos (light)

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t      a1, a2, b1, b2;
semaphore_t s1, s2 ;

// corpo da thread A
void TaskA (void * arg)
{
   int i ;
   for (i=0; i<10; i++)
   {
      sem_down (&s1) ;
      printf ("%s zig (%d)\n", (char *) arg, i) ;
      task_sleep (1000) ;
      sem_up (&s2) ;
   }
   task_exit (0) ;
}

// corpo da thread B
void TaskB (void * arg)
{
   int i ;
   for (i=0; i<10; i++)
   {
      sem_down (&s2) ;
      printf ("%s zag (%d)\n", (char *) arg, i) ;
      task_sleep (1000) ;
      sem_up (&s1) ;
   }
   task_exit (0) ;
}

int main (int argc, char *argv[])
{
   printf ("main: inicio\n") ;

   ppos_init () ;

   // cria semaforos
   sem_create (&s1, 1) ;
   sem_create (&s2, 0) ;

   // cria tarefas
   task_create (&a1, TaskA, "A1") ;
   task_create (&a2, TaskA, "    A2") ;
   task_create (&b1, TaskB, "                         B1") ;
   task_create (&b2, TaskB, "                             B2") ;

   // aguarda a1 encerrar
   task_join (&a1) ;

   // destroi semaforos
   sem_destroy (&s1) ;
   sem_destroy (&s2) ;

   // aguarda a2, b1 e b2 encerrarem
   task_join (&a2) ;
   task_join (&b1) ;
   task_join (&b2) ;

   printf ("main: fim\n") ;
   task_exit (0) ;

   exit (0) ;
}
