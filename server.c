#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

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
	int ser_socket,cli_socket;
	struct sockaddr_in ser_addr, cli_addr;
	socklen_t addr_len = sizeof(cli_addr);

	ser_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(ser_socket == -1)
	{
		perror("Error creating socket");
		exit(1);
	}

	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.s_addr = INADDR_ANY;
	ser_addr.sin_port = htons(PORT);

	if(bind(ser_socket, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) == -1)
	{
		perror("Error in binding socket");
		close(ser_socket);
		exit(1);
	}

	if(listen(ser_socket, 5) == -1)
	{
		perror("Error in listening");
		close(ser_socket);
		exit(1);
	}

	printf("Server is listening on %d\n", PORT);

	while(1)
	{
		cli_socket = accept(ser_socket, (struct sockaddr *)&cli_addr, &addr_len);
		if(cli_socket == -1)
		{
			perror("Error in accepting connection");
			continue;
		}

		printf("Accepted connection from %s   %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

		handle_client(cli_socket);
	}

	close(ser_socket);
	return 0;
}

