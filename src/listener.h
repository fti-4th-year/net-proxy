#pragma once

#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE 0x10000

typedef struct _listener
{
	int id;
	pthread_t thread, reader, writer;
	pthread_mutex_t mutex;
	int active;
	int reader_active, writer_active;
	int cli_sockfd, serv_sockfd;
}
listener;

int init_listener(listener *lst);
int destroy_listener(listener *lst);

int spawn_listener(listener *lst);
int wait_listener(listener *lst);
