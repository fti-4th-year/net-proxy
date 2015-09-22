#pragma once

#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE 0x100

typedef struct _listener
{
	int id;
	pthread_t thread;
	pthread_mutex_t mutex;
	int active;
	int cli_sockfd;
}
listener;

int init_listener(listener *lst)
{
	lst->active = 0;
	pthread_mutex_init(lst->mutex, NULL);
}

int destroy_listener(listener *lst)
{
	pthread_mutex_destroy(lst->mutex);
}

int spawn_listener(listener *lst)
{
	lst->active = 1;
	return pthread_create(lst->thread, NULL, listener_main, (void *) lst);
}

int wait_listener(listener *lst)
{
	int ret = 0;
	if(lst->active)
	{
		lst->active = 0;
		ret = pthread_join(lst->thread, NULL);
	}
	return ret;
}
