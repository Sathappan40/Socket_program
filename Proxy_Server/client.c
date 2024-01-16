#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define P_SERVER_IP "127.0.0.1" 
#define P_SERVER_PORT 8888       
#define MAX_BUFFER_SIZE 4096

int main()
{
	int cli_socket;
	struct sockaddr_in p_ser_addr;
	char request[MAX_BUFFER_SIZE];

	if((cli_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Socket creation failed");
		exit(1);
	}

	p_ser_addr.sin_family = AF_INET;
	p_ser_addr.sin_port = htons(P_SERVER_PORT);
	p_ser_addr.sin_addr.s_addr = inet_addr(P_SERVER_IP);

	if(connect(cli_socket, (struct sockaddr *)&p_ser_addr, sizeof(p_ser_addr))==-1)
	{
		perror("connection to proxy server failed");
		exit(1);
	}

	printf("Enter the request :");
	fgets(request, sizeof(request), stdin);
	
	ssize_t s_size = send(cli_socket, request, strlen(request), 0);
	
	char buffer[MAX_BUFFER_SIZE];
	int r_size;

	while((r_size = recv(cli_socket, buffer, sizeof(buffer), 0)) > 0)
	{
		printf("Received response from proxy : %s\n", buffer);
	}

	close(cli_socket);
	return 0;
}


