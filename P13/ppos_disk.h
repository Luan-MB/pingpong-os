// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

#include "ppos_data.h"

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

typedef enum {READ, WRITE} request_type;

typedef struct request_t {
	struct request_t *next, *prev;
	task_t *req_task;
	request_type req_type;
	int req_block;
	void *req_buffer;
} request_t;

// estrutura que representa um disco no sistema operacional

typedef struct
{
	task_t *disk_task_queue;
	request_t *disk_req_queue;
  	semaphore_t disk_sem;
	int disk_signal;
	request_t *disk_request;
  // completar com os campos necessarios
} disk_t ;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) ;

#endif
