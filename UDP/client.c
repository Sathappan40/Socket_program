#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
	int cli_socket;
	struct sockaddr_in ser_addr;
	char message[BUFFER_SIZE];
	
	cli_socket=socket(AF_INET, SOCK_DGRAM, 0);
	if(cli_socket==-1)
	{
		perror("Error in creating socket");
		exit(1);
	}
	
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	ser_addr.sin_port = htons(PORT);
        
	printf("Enter the message :");
	fgets(message, sizeof(message), stdin);

	ssize_t s_size = sendto(cli_socket, message, strlen(message), 0, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
	
	if(s_size == -1)
	{
		perror("Error in sending message to server");
		close(cli_socket);
		exit(1);
	}
	
	char buffer[BUFFER_SIZE];
	ssize_t r_size = recvfrom(cli_socket, buffer, sizeof(buffer), 0, NULL,NULL);
	if(r_size == -1)
	{
		perror("Error in receiving response from server");
		close(cli_socket);
		exit(1);
	}
	
	buffer[r_size] = '\0';
	printf("Server Response : %s\n", buffer);

	close(cli_socket);

	return 0;
}



