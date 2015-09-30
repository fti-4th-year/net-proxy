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
void *reader_main(void *cookie);
void *writer_main(void *cookie);

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

int parse_socks4(int sockfd, struct sockaddr_in *addr)
{
	int len;
	unsigned char data[9];
	int portno;
	
	len = read(sockfd, data, 9);
	if(data[0] == 4 && len == 9)
	{
		portno = (unsigned short) data[2] << 8 | (unsigned short) data[3];
		addr->sin_addr.s_addr = (int) data[4] | (int) data[5] << 8 | (int) data[6] << 16 | (int) data[7] << 24;
		addr->sin_port = htons(portno);
		
		data[0] = 0;
		data[1] = 0x5a;
		
		len = write(sockfd, data, 8);
		if(len < 8)
		{
			fprintf(stderr, "Error write socks4 data\n");
			return 2;
		}
	}
	else
	{
		fprintf(stderr, "Error read socks4 data\n");
		return 1;
	}
	
	return 0;
}

void *listener_main(void *cookie)
{
	struct sockaddr_in serv_addr;
	listener *lst = (listener *) cookie;
	
	printf("[ %02d ]\tstarted\n", lst->id);
	
	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	
	if(parse_socks4(lst->cli_sockfd, &serv_addr) != 0)
		goto finalize;
	
	lst->serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(lst->serv_sockfd < 0)
		perror("Error opening server socket");
	
	if(connect(lst->serv_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		perror("Error connecting to server");
	
	lst->reader_active = 1;
	pthread_create(&lst->reader, NULL, reader_main, (void *) lst);
	lst->writer_active = 1;
	pthread_create(&lst->writer, NULL, writer_main, (void *) lst);
	
	int done = 0;
	for(;;)
	{
		pthread_mutex_lock(&lst->mutex);
		{
			if(!lst->active || !lst->reader_active || !lst->writer_active)
				done = 1;
		}
		pthread_mutex_unlock(&lst->mutex);
		
		if(done)
			break;
		
		sleep(1);
	}
	
	close(lst->serv_sockfd);
	printf("[ %02d ]\tdone\n", lst->id);
	
finalize:
	close(lst->cli_sockfd);
	
	lst->reader_active = 0;
	pthread_join(lst->reader, NULL);
	lst->writer_active = 0;
	pthread_join(lst->writer, NULL);
	
	pthread_mutex_lock(&lst->mutex);
	{
		lst->active = 0;
	}
	pthread_mutex_unlock(&lst->mutex);
	
	printf("[ %02d ]\tstopped\n", lst->id);
	
	return NULL;
}


void *reader_main(void *cookie) 
{
	int len;
	char *buffer = (char *) malloc(BUFFER_SIZE);
	listener *lst = (listener *) cookie;
	
	if(buffer == NULL)
	{
		fprintf(stderr, "cannot allocate memory\n");
		goto finalize;
	}
	
	int done = 0;
	for(;;)
	{
		pthread_mutex_lock(&lst->mutex);
		{
			if(!lst->reader_active)
				done = 1;
		}
		pthread_mutex_unlock(&lst->mutex);
		
		if(done)
			break;
		
		len = read(lst->serv_sockfd, buffer, BUFFER_SIZE);
		printf("[ %02d ]\tserver read %d bytes\n", lst->id, len);
		if(len < 0)
		{
			fprintf(stderr, "[ %02d ]\terror read server socket", lst->id);
			perror("");
		}
		if(len <= 0)
			break;
		
		len = write(lst->cli_sockfd, buffer, len);
		printf("[ %02d ]\tclient write %d bytes\n", lst->id, len);
		if(len < 0)
		{
			fprintf(stderr, "[ %02d ]\terror write client socket", lst->id);
			perror("");
		}
		if(len <= 0)
			break;
	}
	
	free(buffer);
	
finalize:
	pthread_mutex_lock(&lst->mutex);
	{
		lst->reader_active = 0;
	}
	pthread_mutex_unlock(&lst->mutex);
	
	return NULL;
}

void *writer_main(void *cookie) 
{
	int len;
	char *buffer = (char *) malloc(BUFFER_SIZE);
	listener *lst = (listener *) cookie;
	
	if(buffer == NULL)
	{
		fprintf(stderr, "cannot allocate memory\n");
		goto finalize;
	}
	
	int done = 0;
	for(;;)
	{
		pthread_mutex_lock(&lst->mutex);
		{
			if(!lst->writer_active)
				done = 1;
		}
		pthread_mutex_unlock(&lst->mutex);
		
		if(done)
			break;
		
		len = read(lst->cli_sockfd, buffer, BUFFER_SIZE);
		printf("[ %02d ]\tclient read %d bytes\n", lst->id, len);
		if(len < 0)
		{
			fprintf(stderr, "[ %02d ]\terror read client socket", lst->id);
			perror("");
		}
		if(len <= 0)
			break;
		
		//write(0, buffer, len);
		len = write(lst->serv_sockfd, buffer, len);
		printf("[ %02d ]\tserver write %d bytes\n", lst->id, len);
		if(len < 0)
		{
			fprintf(stderr, "[ %02d ]\terror write server socket", lst->id);
			perror("");
		}
		if(len <= 0)
			break;
	}
	
	free(buffer);
	
finalize:
	pthread_mutex_lock(&lst->mutex);
	{
		lst->writer_active = 0;
	}
	pthread_mutex_unlock(&lst->mutex);
	
	return NULL;
}
