#pragma once

#include "octet.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

typedef struct task_t {
	int client_sock;
	struct sockaddr_in client_addr;
} task_t;

typedef struct queue_t {
	task_t *tasks;
	atomic_uint_fast8_t head;
	atomic_uint_fast8_t tail;
	atomic_uint_fast8_t size;
	pthread_mutex_t lock;
	pthread_cond_t filled;
	pthread_cond_t available;
} queue_t;

extern struct queue_t queue;

typedef struct arg_t {
	uint8_t id;
	octet_t db;
	char *database_buffer;
	char *request_buffer;
	char *response_buffer;
} arg_t;

typedef struct worker_t {
	arg_t arg;
	pthread_t thread;
} worker_t;

typedef struct thread_pool_t {
	pthread_t scaler;
	worker_t *workers;
	atomic_uint_fast8_t size;
	atomic_uint_fast8_t load;
	pthread_mutex_t lock;
	pthread_cond_t scale;
	pthread_cond_t available;
	atomic_bool stopping;
} thread_pool_t;

extern struct thread_pool_t thread_pool;

int spawn(worker_t *worker, uint8_t id, void *(*function)(void *),
					void (*logger)(const char *message, ...) __attribute__((format(printf, 1, 2))));

int join(worker_t *worker, uint8_t id);

void *thread(void *args);

void *scaler(void *args);
