#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include<sys/wait.h>
#include<errno.h>
#include <signal.h>

#define SERVER_PORT 8080
#define MAX_BUFFER_SIZE 4096

int main()
{
	int ser_socket, cli_socket;
	struct sockaddr_in ser_addr, cli_addr;
	socklen_t cli_addr_len = sizeof(cli_addr);
	
	if((ser_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket creation failed");
		exit(1);
	}

	memset(&ser_addr, 0, sizeof(&ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(SERVER_PORT);
	ser_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(ser_socket, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) == -1)
	{
		perror("Binding failed");
		exit(1);
	}

	if(listen(ser_socket, 5) == -1)
	{
		perror("Error in listening");
		close(ser_socket);
		exit(1);
	}

	printf("Server listening on port %d\n", SERVER_PORT);

	while(1)
	{
		if((cli_socket = accept(ser_socket, (struct sockaddr *)&cli_addr, &cli_addr_len)) == -1)
		{
			perror("Accepting connection failed");
			continue;
		}

		char buffer[MAX_BUFFER_SIZE];
		int r_size;

		if((r_size = recv(cli_socket, buffer, sizeof(buffer), 0)) <= 0)
		{
			perror("Error receiving data");
			exit(1);
		}
		
		printf("Received request from client : %s", buffer);
		
		char response[MAX_BUFFER_SIZE];
		printf("Enter the response :");
		fgets(response, sizeof(response), stdin);
	
		ssize_t s_size = send(cli_socket, response, strlen(response), 0);
		close(cli_socket);
	}
	close(ser_socket);
	return 0;
}


