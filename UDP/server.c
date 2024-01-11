#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
	int ser_socket;
	struct sockaddr_in ser_addr, cli_addr;
	socklen_t addr_len = sizeof(cli_addr);
	char buffer[BUFFER_SIZE];
	char message[10];

	ser_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(ser_socket == -1)
	{
		perror("Error creating socket");
		exit(1);
	}

	ser_addr.sin_family = AF_INET;
        ser_addr.sin_addr.s_addr = INADDR_ANY;
        ser_addr.sin_port = htons(PORT);

	if(bind(ser_socket, (struct sockaddr *)&ser_addr, sizeof(ser_addr))==-1)
	{
		perror("Error binding socket");
		close(ser_socket);
		exit(1);
	}

	printf("Server lisetinig on port %d\n", PORT);

	while(1)
	{
		ssize_t r_size = recvfrom(ser_socket, buffer, sizeof(buffer), 0,(struct sockaddr *)&cli_addr, &addr_len);

		if(r_size <= 0)
		{
			perror("Error receiving message from client");
			continue;
		}

		buffer[r_size] = '\0';
		printf("Received message from client : %s\n", buffer);

		time_t rawtime;
		struct tm *timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		char timestamp[64];
		strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
               
                printf("Enter the message :");
	        fgets(message, sizeof(message), stdin); 
		snprintf(buffer, sizeof(buffer), " %s %s ",message,timestamp);
		
		ssize_t s_size = sendto(ser_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, addr_len);

		if(s_size <= 0)
		{
			perror("Error sending response to client");
		}
	}
	close(ser_socket);
	return 0;
}

