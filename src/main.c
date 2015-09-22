#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "listener.h"

#define POOL_SIZE   0x10

int main(int argc, char *argv[])
{
	int ret = 0;
	int proxy_sockfd;
	struct sockaddr_in proxy_addr, cli_addr;
	socklen_t clilen;
	int proxy_portno;
	listener pool[POOL_SIZE];
	int i;
	
	for(i = 0; i < POOL_SIZE; ++i)
	{
		pool[i].active = 0;
		pthread_mutex_init(pool[i].mutex, NULL);
	}
	
	proxy_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(proxy_sockfd < 0) 
	{
		perror("Error opening proxy socket");
		ret = 1;
		goto finalize;
	}
	
	bzero((char *) &proxy_addr, sizeof(proxy_addr));
	proxy_portno = 8080;
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
	
	for(;;)
	{
		int spawned = 0;
		int cli_sockfd = 0;
		struct timeval tv;
		fd_set set;
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		
		FD_ZERO(&set);
		FD_SET(proxy_sockfd, &set);
		if(select(proxy_sockfd + 1, &set, NULL, NULL, &tv) > 0)
		{
			cli_sockfd = accept(proxy_sockfd, (struct sockaddr *) &cli_addr, &clilen);
			if(cli_sockfd < 0)
			{
				perror("Error on accept client socket");
				goto join_listeners;
			}
		}
		
		for(i = 0; i < POOL_SIZE; ++i)
		{
			spawned = 0;
			pthread_mutex_lock(pool[i].mutex);
			{
				if(!pool[i].active)
				{
					pool[i].cli_sockfd = cli_sockfd;
					pool[i].serv_sockfd = serv_sockfd;
					spawn_listener(pool + i);
					spawned = 1;
				}
			}
			pthread_mutex_unlock(pool[i].mutex);
			if(spawned)
				break;
		}
		if(spawned)
			continue;
		
		
	}
	
join_listeners:
	for(i = 0; i < POOL_SIZE; ++i)
	{
		wait_listener(pool[i]);
	}

close_proxy:
	close(proxy_sockfd);
	
finalize:
	for(i = 0; i < POOL_SIZE; ++i)
	{
		pthread_mutex_destroy(pool[i].mutex);
	}
	
	return ret;
}
