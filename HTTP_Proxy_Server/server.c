#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include<sys/wait.h>
#include<errno.h>
#include <signal.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 4096

void handle_client(int cli_socket, char* req_method, char* req_body)
{
	char response_header[BUFFER_SIZE];
	char response_body[BUFFER_SIZE];

	if(strcmp(req_method, "GET") == 0)
	{
		sprintf(response_body, "<html><body><h1>Hello, this is the server response for GET request!</h1></body></html>\r\n");
    	}
       	else if (strcmp(req_method, "POST") == 0)
       	{
        	sprintf(response_body, "<html><body><h1>Hello, this is the server response for POST request!</h1><p>Received POST data: %s</p></body></html>\r\n", req_body);
    	}
       	else
       	{
        	sprintf(response_body, "<html><body><h1>Unsupported request method!</h1></body></html>\r\n");
    	}

	sprintf(response_header, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n", strlen(response_body));

    	send(cli_socket, response_header, strlen(response_header), 0);
    	send(cli_socket, response_body, strlen(response_body), 0);

    	close(cli_socket);
}

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
			perror("Error accepting connection");
			exit(1);
		}

		char request[BUFFER_SIZE];
		recv(cli_socket, request, BUFFER_SIZE, 0);

		char req_method[BUFFER_SIZE];
		char req_body[BUFFER_SIZE];
		sscanf(request, "%s %*s %*s\r\n\r\n%[^\r]", req_method, req_body);

		handle_client(cli_socket, req_method, req_body);
	}

	close(ser_socket);
	return 0;
}

