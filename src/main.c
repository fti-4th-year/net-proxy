#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>

#include "listener.h"

#define POOL_SIZE   0x100

int done = 0;

void sighandler(int signo)
{
	done = 1;
}

void sigpipehandler(int signo)
{
	const char *pmsg = "trying to write to broken socket\n";
	write(0, pmsg, strlen(pmsg));
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int proxy_sockfd;
	struct sockaddr_in proxy_addr, cli_addr;
	socklen_t clilen;
	int proxy_portno;
	listener pool[POOL_SIZE];
	int i;
	
	printf("server started\n");
	
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGPIPE, sigpipehandler);
	
	for(i = 0; i < POOL_SIZE; ++i)
	{
		pool[i].id = i;
		init_listener(pool + i);
	}
	
	proxy_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(proxy_sockfd < 0) 
	{
		perror("Error opening proxy socket");
		ret = 1;
		goto finalize;
	}
	
	bzero((char *) &proxy_addr, sizeof(proxy_addr));
	proxy_portno = 8081;
	proxy_addr.sin_family = AF_INET;
	proxy_addr.sin_addr.s_addr = INADDR_ANY;
	proxy_addr.sin_port = htons(proxy_portno);
	if(bind(proxy_sockfd, (struct sockaddr *) &proxy_addr, sizeof(proxy_addr)) < 0) 
	{
		perror("Error on binding proxy socket");
		ret = 2;
		goto close_proxy;
	}
	
	listen(proxy_sockfd,5);
	clilen = sizeof(cli_addr);
	
	while(!done)
	{
		int status;
		int spawned = 0;
		int cli_sockfd = 0;
		struct timeval tv;
		fd_set set;
		tv.tv_sec = 0;
		tv.tv_usec = 10000;
		
		FD_ZERO(&set);
		FD_SET(proxy_sockfd, &set);
		status = select(proxy_sockfd + 1, &set, NULL, NULL, &tv);
		if(status > 0)
		{
			cli_sockfd = accept(proxy_sockfd, (struct sockaddr *) &cli_addr, &clilen);
			if(cli_sockfd < 0)
			{
				perror("Error on accept client socket");
				goto join_listeners;
			}
			
			int p_host = cli_addr.sin_addr.s_addr;
			int p_port = cli_addr.sin_port;
			printf("accepted %d.%d.%d.%d:%d\n", 
			       (p_host >>  0) & 0xff,
			       (p_host >>  8) & 0xff,
			       (p_host >> 16) & 0xff,
			       (p_host >> 24) & 0xff,
			       p_port
			       );
		}
		else
		if(status == 0)
			continue;
		else
		{
			perror("Error select proxy");
			goto join_listeners;
		}
		
		for(i = 0; i < POOL_SIZE; ++i)
		{
			spawned = 0;
			pthread_mutex_lock(&pool[i].mutex);
			{
				if(!pool[i].active)
				{
					pool[i].cli_sockfd = cli_sockfd;
					spawn_listener(pool + i);
					spawned = 1;
				}
			}
			pthread_mutex_unlock(&pool[i].mutex);
			if(spawned)
				break;
		}
		if(spawned)
			continue;
		
		fprintf(stderr, "thread pool is full\ntry again\n");
	}
	
join_listeners:
	for(i = 0; i < POOL_SIZE; ++i)
	{
		int active = 0;
		pthread_mutex_lock(&pool[i].mutex);
		{
			if(pool[i].active)
			{
				active = 1;
				pool[i].active = 0;
			}
		}
		pthread_mutex_unlock(&pool[i].mutex);
		if(active)
			wait_listener(pool + i);
	}

close_proxy:
	close(proxy_sockfd);
	
finalize:
	for(i = 0; i < POOL_SIZE; ++i)
	{
		destroy_listener(pool + i);
	}
	
	printf("server stopped\n");
	
	return ret;
}
