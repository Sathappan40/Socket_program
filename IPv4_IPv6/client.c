#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define SERVER_IP "127.0.0.1"
#define PORT "8080"
#define BUFFER_SIZE 1024

int main()
{
	int cli_socket;
	struct addrinfo hints, *servinfo, *p;
	int v;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((v = getaddrinfo(SERVER_IP, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr,"getaddrinfo : %s\n", gai_strerror(v));
		return 1;
	}

	for(p=servinfo; p!=NULL; p = p->ai_next)
	{
		cli_socket = socket(p->ai_family,p->ai_socktype,p->ai_protocol);			
		if(cli_socket == -1)
		{
			perror("Error in creating socket");
			continue;
		}

		if(connect(cli_socket, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(cli_socket);
			perror("Error in connecting");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo);

	if(p == NULL)
	{
		fprintf(stderr, "client : Failed to connect");
		exit(1);
	}

	char message[BUFFER_SIZE];
	printf("Enter the message");
	fgets(message, sizeof(message), stdin);

	ssize_t s_size = send(cli_socket, message, strlen(message), 0);

	if(s_size == -1)
	{
		perror("Error in sending the message");
		close(cli_socket);
		exit(1);
	}

	char buffer[BUFFER_SIZE];
	ssize_t r_size = recv(cli_socket, buffer, sizeof(buffer), 0);

	if(r_size == -1)
	{
		perror("Error in receiving the message forom server");
		close(cli_socket);
		exit(1);
	}

	buffer[r_size] = '\0';
	printf("Server Response : %s", buffer);

	close(cli_socket);
	return 0;
}


