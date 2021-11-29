// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Teste do task dispatcher e escalonador FCFS

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

task_t Pang, Peng, Ping, Pong, Pung ;

// corpo das threads
void Body (void * arg)
{
   int i ;

   printf ("%s: inicio\n", (char *) arg) ;
   for (i=0; i<5; i++)
   {
      printf ("%s: %d\n", (char *) arg, i) ;
      task_yield ();
   }
   printf ("%s: fim\n", (char *) arg) ;
   task_exit (0) ;
}

int main (int argc, char *argv[])
{
   printf ("main: inicio\n");

   ppos_init () ;

   task_create (&Pang, Body, "    Pang") ;
   task_create (&Peng, Body, "        Peng") ;
   task_create (&Ping, Body, "            Ping") ;
   task_create (&Pong, Body, "                Pong") ;
   task_create (&Pung, Body, "                    Pung") ;

   task_yield () ;

   printf ("main: fim\n");
   exit (0);
}
