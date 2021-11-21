// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Teste de filas de mensagens

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "ppos.h"

task_t prod[3], somador, cons[2] ;
mqueue_t queueValores, queueRaizes ;

// corpo da thread produtor
void prodBody (void * saida)
{
   int valor ;

   while (1)
   {
      // sorteia um valor inteiro aleatorio
      valor = random() % 1000 ;

      // envia o valor inteiro na fila de saida
      if (mqueue_send (&queueValores, &valor) < 0)
      {
         printf ("T%d terminou\n", task_id()) ;
         task_exit (0) ;
      }

      printf ("T%d enviou %d\n", task_id(), valor) ;

      // dorme um intervalo aleatorio
      task_sleep (random() % 3000) ;
   }
}

// corpo da thread somador
void somaBody (void * arg)
{
   int v1, v2, v3, i ;
   double soma, raiz ;

   for (i=0; i<10; i++)
   {
      // recebe tres valores inteiros
      mqueue_recv (&queueValores, &v1) ;
      printf ("               T%d: recebeu %d\n", task_id(), v1) ;
      mqueue_recv (&queueValores, &v2) ;
      printf ("               T%d: recebeu %d\n", task_id(), v2) ;
      mqueue_recv (&queueValores, &v3) ;
      printf ("               T%d: recebeu %d\n", task_id(), v3) ;

      // calcula a soma e sua raiz
      soma = v1 + v2 + v3 ;
      raiz = sqrt (soma) ;
      printf ("               T%d: %d+%d+%d = %f (raiz %f)\n",
              task_id(), v1, v2, v3, soma, raiz) ;

      // envia a raiz da soma
      mqueue_send (&queueRaizes, &raiz) ;

      // dorme um intervalo aleatorio
      task_sleep (random() % 3000) ;
   }
   task_exit(0) ;
}

// corpo da thread consumidor
void consBody (void * arg)
{
   double valor ;

   while(1)
   {
      // recebe um valor (double)
      if (mqueue_recv (&queueRaizes, &valor) < 0)
      {
         printf ("                                 T%d terminou\n",
                 task_id()) ;
         task_exit (0) ;
      }
      printf ("                                 T%d consumiu %f\n",
              task_id(), valor) ;

      // dorme um intervalo aleatorio
      task_sleep (random() % 3000) ;
   }
}

int main (int argc, char *argv[])
{
   printf ("main: inicio\n") ;

   ppos_init () ;

   // cria as filas de mensagens (5 valores cada)
   mqueue_create (&queueValores, 5, sizeof(int)) ;
   mqueue_create (&queueRaizes,  5, sizeof(double)) ;

   // cria as threads
   task_create (&somador, somaBody, NULL) ;
   task_create (&cons[0], consBody, NULL) ;
   task_create (&cons[1], consBody, NULL) ;
   task_create (&prod[0], prodBody, NULL) ;
   task_create (&prod[1], prodBody, NULL) ;
   task_create (&prod[2], prodBody, NULL) ;

   // aguarda o somador encerrar
   task_join (&somador) ;

   // destroi as filas de mensagens
   printf ("main: destroi queueValores\n") ;
   mqueue_destroy (&queueValores) ;
   printf ("main: destroi queueRaizes\n") ;
   mqueue_destroy (&queueRaizes) ;

   // encerra a thread main
   printf ("main: fim\n") ;
   task_exit (0) ;

   exit (0) ;
}
