#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PROXY_PORT 8888
#define MAX_BUFFER_SIZE 4096
#define SERVER_IP "127.0.0.1"  
#define SERVER_PORT 8080 

void handle_client(int cli_socket);

int main()
{
	int p_socket, cli_socket;
	struct sockaddr_in p_addr, cli_addr;
	socklen_t cli_addr_len = sizeof(cli_addr);

	if((p_socket = socket(AF_INET, SOCK_STREAM, 0))==-1)
	{
		perror("Socket creation failed");
		exit(1);
	}

	p_addr.sin_family = AF_INET;
	p_addr.sin_port = htons(PROXY_PORT);
	p_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(p_socket, (struct sockaddr *)&p_addr, sizeof(p_addr)) == -1)
	{
		perror("Binding failed");
		exit(1);
	}

	if(listen(p_socket, 10) == -1)
	{
		perror("Listening Failed");
		exit(1);
	}

	printf("Proxy Server is listening on port %d \n", PROXY_PORT);

	while(1)
	{
		if((cli_socket = accept(p_socket, (struct sockaddr *)&cli_addr, &cli_addr_len)) == -1)
	        {
			perror("Acepting connection failed");
			continue;
		}

		handle_client(cli_socket);
		close(cli_socket);
	}

	close(p_socket);
	return 0;
}

void handle_client(int cli_socket)
{
	char buffer[MAX_BUFFER_SIZE];
	int r_size;

	if((r_size = recv(cli_socket, buffer, sizeof(buffer), 0)) <= 0)
	{
		perror("Error reading data");
		return ;
	}

	printf("Received request from client : %s ", buffer);

	int ser_socket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in ser_addr;
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(SERVER_PORT);
	ser_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	if(connect(ser_socket, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) == -1)
	{
		perror("Error connecting to server");
		return ;
	}
	
	int s_size;
	s_size = send(ser_socket, buffer, r_size, 0);
	if(s_size <= 0)
	{
		perror("Error sending data to the server");
		return ;
	}

	int r_size2, s_size2;
	while((r_size2 = recv(ser_socket, buffer, sizeof(buffer), 0)) > 0)
	{
		printf("Received response from the server is : %s", buffer);
		s_size2 = send(cli_socket, buffer, r_size2, 0);
		if(s_size2 <= 0)
		{
			perror("Error sending the data to client");
			break;
		}
	}

	close(ser_socket);
}


