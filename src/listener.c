#include "listener.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void *listener_main(void *cookie);

int init_listener(listener *lst)
{
	lst->active = 0;
	pthread_mutex_init(&lst->mutex, NULL);
}

int destroy_listener(listener *lst)
{
	pthread_mutex_destroy(&lst->mutex);
}

int spawn_listener(listener *lst)
{
	lst->active = 1;
	return pthread_create(&lst->thread, NULL, listener_main, (void *) lst);
}

int wait_listener(listener *lst)
{
	lst->active = 0;
	return pthread_join(lst->thread, NULL);
}

void *listener_main(void *cookie)
{
	listener *lst = (listener *) cookie;
	int cli_sockfd = lst->cli_sockfd;
	
	int serv_sockfd, serv_portno;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
	printf("thread%d started\n", lst->id);
	
	serv_portno = 80;
	serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(serv_sockfd < 0)
		perror("Error opening server socket");
	
	server = gethostbyname("localhost");
	if(server == NULL)
		perror("Error, no such server host");
	
	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*)server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(serv_portno);
	
	if(connect(serv_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		perror("Error connecting to server");
	
	for(;;)
	{
		struct timeval tv;
		fd_set set;
		tv.tv_sec = 0;
		tv.tv_usec = 10000;
		
		int status;
		int len;
		char buffer[BUFFER_SIZE];
		
		int done = 0;
		int cli_to = 0, serv_to = 0;
		
		pthread_mutex_lock(&lst->mutex);
		{
			if(!lst->active)
				done = 1;
		}
		pthread_mutex_unlock(&lst->mutex);
		if(done)
			break;
		
		FD_ZERO(&set);
		FD_SET(cli_sockfd, &set);
		status = select(cli_sockfd + 1, &set, NULL, NULL, &tv);
		if(status > 0)
		{
			len = read(cli_sockfd, buffer, BUFFER_SIZE);
			if(len < 0)
			{
				perror("Error read client socket");
				break;
			}
			//write(0, buffer, len);
			len = write(serv_sockfd, buffer, len);
			if(len < 0)
			{
				perror("Error write server socket");
				break;
			}
		}
		else
		if(status == 0)
		{
			cli_to = 1;
		}
		else
		{
			perror("Error select client");
			break;
		}
		
		
		FD_ZERO(&set);
		FD_SET(serv_sockfd, &set);
		status = select(serv_sockfd + 1, &set, NULL, NULL, &tv);
		if(status > 0)
		{
			len = read(serv_sockfd, buffer, BUFFER_SIZE);
			if(len < 0)
			{
				perror("Error read server socket");
				break;
			}
			len = write(cli_sockfd, buffer, len);
			if(len < 0)
			{
				perror("Error write client socket");
				break;
			}
		}
		else
		if(status == 0)
		{
			serv_to = 1;
		}
		else
		{
			perror("Error select server");
			break;
		}
		
		if(cli_to && serv_to)
			break;
	}
	
	close(serv_sockfd);
	close(cli_sockfd);
	
	pthread_mutex_lock(&lst->mutex);
	{
		lst->active = 0;
	}
	pthread_mutex_unlock(&lst->mutex);
	
	printf("thread%d stopped\n", lst->id);
	
	return NULL;
}
