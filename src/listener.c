#include "listener.h"

void listener_main(void *cookie)
{
	listener *lst = (listener *) cookie;
	int cli_sockfd = lst->cli_sockfd;
	
	int serv_sockfd, serv_portno;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
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
	
	while(lst->active) 
	{
		struct timeval tv;
		fd_set set;
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		
		int len;
		char buffer[BUFFER_SIZE];
		
		FD_ZERO(&set);
		FD_SET(cli_sockfd, &set);
		if(select(cli_sockfd + 1, &set, NULL, NULL, &tv) > 0)
		{
			len = read(cli_sockfd, buffer, BUFFER_SIZE);
			if(len < 0)
			{
				perror("Error read client socket");
				break;
			}
			write(0, buffer, len);
			len = write(serv_sockfd, buffer, len);
			if(len < 0)
			{
				perror("Error write server socket");
				break;
			}
		}
		
		FD_ZERO(&set);
		FD_SET(serv_sockfd, &set);
		if(select(serv_sockfd + 1, &set, NULL, NULL, &tv) > 0)
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
	}
	
	close(serv_sockfd);
	close(cli_sockfd);
	
	pthread_mutex_lock(lst->mutex);
	{
		lst->active = 0;
	}
	pthread_mutex_unlock(lst->mutex);
}
