#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>

#define PORT "8080"
#define BUFFER_SIZE 4096

void handle_client(int cli_socket)
{
	char buffer[BUFFER_SIZE];
	char message[BUFFER_SIZE];
	time_t rawtime;
	struct tm *timeinfo;
	
	ssize_t r_size = recv(cli_socket, buffer, sizeof(buffer), 0);
	if(r_size <= 0)
	{
		perror("Error receiving message from client");
		close(cli_socket);
		return;
	}

	buffer[r_size] = "\0";
	printf("Received message from client : %s\n", buffer);
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char timestamp[64];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

	printf("Enter the message :");
	fgets(message, sizeof(message), stdin);

	snprintf(buffer, sizeof(buffer), "Server response :  %s %s",message, timestamp);

	ssize_t s_size = send(cli_socket, buffer, strlen(buffer), 0);
	if(s_size <= 0)
	{
		perror("Error sending response to client");
	}

	close(cli_socket);
}

int main()
{
	int ser_socket;
	struct addrinfo hints, *servinfo, *p;
	int v;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((v = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo error : %s\n", gai_strerror(v));
		return 1;
	}

	for(p=servinfo; p != NULL; p = p->ai_next)
	{
		ser_socket = socket(p->ai_family,p->ai_socktype,p->ai_protocol);		
		if(ser_socket == -1)
		{
			perror("Error in creating socket");
			continue;
		}

		int yes = 1;
		if(setsockopt(ser_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("setsocket");
			exit(1);
		}

		if(bind(ser_socket, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(ser_socket);
			perror("Error in binding");
			continue;
		}

		break;
	}
	
	freeaddrinfo(servinfo);

	if(p == NULL)
	{
		fprintf(stderr, "Server : Failed to bind");
		exit(1);
	}

	if(listen(ser_socket, 10) == -1)
	{
		perror("Error in listening");
		exit(1);
	}

	printf("Server is listening in port %s \n", PORT);

	while(1)
	{
		
		struct sockaddr_storage cli_addr;
		socklen_t addr_size = sizeof cli_addr;
		
		int cli_socket;
		printf("%d\n",cli_socket);
		if((cli_socket = accept(ser_socket, (struct sockaddr *)&cli_addr, &addr_size)) == -1)
		{
			perror("Error in accepting");
			break;
		}

		char cli_ip[INET6_ADDRSTRLEN];
		
		if(cli_addr.ss_family = AF_INET)
		{
			struct sockaddr_in *ipv4=(struct sockaddr_in *)&cli_addr;				
			inet_ntop(AF_INET, &(ipv4->sin_addr), cli_ip, INET_ADDRSTRLEN);
		}
		else
		{
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&cli_addr;
			inet_ntop(AF_INET6, &(ipv6->sin6_addr), cli_ip, INET6_ADDRSTRLEN);
		}
		
		printf("Accepted connection from %s\n", cli_ip);

		handle_client(cli_socket);
	}
	close(ser_socket);
	return 0;
}

