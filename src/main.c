#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char *argv[]) {
	
	int proxy_sockfd;
	struct sockaddr_in proxy_addr, cli_addr;
	socklen_t clilen;
	
	int proxy_portno;
	
	proxy_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(proxy_sockfd < 0) 
	{
		perror("Error opening proxy socket");
		return 1;
	}
	
	bzero((char *) &proxy_addr, sizeof(proxy_addr));
	proxy_portno = 8080;
	proxy_addr.sin_family = AF_INET;
	proxy_addr.sin_addr.s_addr = INADDR_ANY;
	proxy_addr.sin_port = htons(proxy_portno);
	if(bind(proxy_sockfd, (struct sockaddr *) &proxy_addr, sizeof(proxy_addr)) < 0)
	{
		perror("Error on binding proxy socket");
		return 2;
	}
	
	listen(proxy_sockfd,5);
	clilen = sizeof(cli_addr);
	
	for(;;)
	{
		int cli_sockfd = 0;
		
		cli_sockfd = accept(proxy_sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if(cli_sockfd < 0)
		{
			perror("Error on accept client socket");
			return 3;
		}
		
		// read SOCKS4
		{
			char buf[9];
			int len = read(cli_sockfd, buf, 9);
			if(len < 0)
			{
				perror("Error read SOCKS4 from client");
			}
			write(0, buf, 9);
			/*
			printf("SOCKS:\n");
			printf("version: %d\n", (int) buf[0]);
			printf("command: %d\n", (int) buf[1]);
			printf("port: %d\n", (int) buf[2] | (int) buf[3] << 8);
			printf("address: %d:%d:%d:%d\n", (int) buf[4], (int) buf[5], (int) buf[6], (int) buf[7]);
			*/
		}
		
		int serv_sockfd, serv_portno;
		struct sockaddr_in serv_addr;
		struct hostent *server;
		
		serv_portno = 80;
		serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(serv_sockfd < 0)
		{
			perror("Error opening server socket");
		}
		
		server = gethostbyname("localhost");
		if(server == NULL)
		{
			perror("Error, no such server host");
		}
		
		bzero((char*)&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char*)server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
		serv_addr.sin_port = htons(serv_portno);
		
		if(connect(serv_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		{
			perror("Error connecting to server");
		}
		
		for(;;)
		{
			struct timeval tv;
			fd_set set;
			tv.tv_sec = 0;
			tv.tv_usec = 1000;
			
			int len;
			static const int BUF_SIZE = 0x100;
			char buffer[BUF_SIZE];
			
			FD_ZERO(&set);
			FD_SET(cli_sockfd, &set);
			if(select(cli_sockfd + 1, &set, NULL, NULL, &tv) > 0)
			{
				len = read(cli_sockfd, buffer, BUF_SIZE);
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
				len = read(serv_sockfd, buffer, BUF_SIZE);
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
	}
	
	close(proxy_sockfd);
	
	return 0;
}
