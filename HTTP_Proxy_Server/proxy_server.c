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
int client_creation(char *, char *);
void message_handler(int, int, char*);
void message_handler_http (int, int, char *);
void* get_in_addr(struct sockaddr *);


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

void *get_in_addr (struct sockaddr *sa)
{
	if (sa -> sa_family == AF_INET)
		return &(((struct sockaddr_in *) sa) -> sin_addr);
		
	return &(((struct sockaddr_in6 *) sa) -> sin6_addr);
}

void message_handler (int client_socket, int destination_socket, char data_buffer [])
{
	struct pollfd pollfds [2];
    pollfds [0].fd = client_socket;
    pollfds [0].events = POLLIN;
    pollfds [0].revents = 0;
    pollfds [1].fd = destination_socket;
    pollfds [1].events = POLLIN;
    pollfds [1].revents = 0;
	ssize_t n;

	while (1)
	{
		if (poll (pollfds, 2, -1) == -1)
		{
			perror ("poll");
			exit (1);
		}
		
		
		for (int fd = 0; fd < 2; fd++)
		{
			//Message from client to server
			if (pollfds [fd].revents & POLLIN && !fd)
			{
				n = read (pollfds [0].fd, data_buffer, 1024);
				if (n <= 0)
					return;

				data_buffer [n] = '\0';

				write (pollfds [1].fd, data_buffer, n);
			}
			
			//Message from server to client
			if (pollfds [fd].revents & POLLIN && fd)
			{
				n = read (pollfds [1].fd, data_buffer, 1024);
				if (n <= 0)
					return;

				data_buffer [n] = '\0';

				write (pollfds [0].fd, data_buffer, n);
			}
		}
	}
}

void message_handler_http (int client_socket, int destination_socket, char data[])
{
	ssize_t n;
	n = write (destination_socket, data, 2048);
	
	while ((n = recv (destination_socket, data, 2048, 0)) > 0)
		send (client_socket, data, n, 0);
}

int client_creation (char* port, char* destination_server_addr)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	char s [INET6_ADDRSTRLEN];
	int rv;

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo (destination_server_addr, port, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n",gai_strerror(rv));	
		return -1;
	}
	
	/*struct sockaddr_in proxy_addr;
	memset (&proxy_addr, 0, sizeof (proxy_addr));
	proxy_addr.sin_family = AF_INET;
	proxy_addr.sin_port = htons (10000);
	proxy_addr.sin_addr.s_addr = INADDR_ANY;*/

	for (p = servinfo; p != NULL; p = p -> ai_next)
	{
		sockfd = socket (p -> ai_family, p -> ai_socktype, p -> ai_protocol);
		if (sockfd == -1)
		{ 
			perror ("client: socket\n"); 
			continue; 
		}
		
		int yes = 1;
		
		if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror ("setsockopt");
			exit (1);	
		}
		
		// connect will help us to connect to the server with the addr given in arguments.
		if (connect (sockfd, p -> ai_addr, p -> ai_addrlen) == -1) 
		{
			close(sockfd);
			perror("client: connect");
			continue;
		} 
		break;
	}

	if (p == NULL)
	{
		fprintf (stderr, "client: failed to connect\n");
		return -1;	
	}
	
	//printing ip address of the server.
	inet_ntop (p -> ai_family, get_in_addr ((struct sockaddr *)p -> ai_addr), s, sizeof (s));
	
	printf ("proxy client: connecting to %s\n", s);
	freeaddrinfo (servinfo);
	
	return sockfd;
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
	
	char method[16];
	char host[256]
	char buffer1[4096];
	strcpy(buffer1, buffer);
	printf ("%s\n", buffer1);
	sscanf (buffer, "%s %s", method, host);
	
	if (strcmp (method, "CONNECT") == 0) 
        {
        	char *port_str = strchr (host, ':');
        	char https_port [10] = "443";
        	char *port;
        
		if (port_str != NULL) 
		{
		    *port_str = '\0';
		    port = port_str + 1;
		}
		else
			port = https_port;
		
		int destination_socket = client_creation (port, host);
		if (destination_socket == -1) 
		{
		    perror ("socket");
		    close (cli_socket);
		    exit (EXIT_FAILURE);
		}
		const char *response = "HTTP/1.1 200 Connection established\r\n\r\n";
	        int r = write (cli_socket, response, strlen (response));
	
        	message_handler (cli_socket, destination_socket, buffer1);
        }
        
       else
       {
       	char *host_str = strstr (buffer, "Host: ") + 6;
		char *host_end = strstr (host_str, "\r\n");
		*host_end = '\0';
		
		char* port;
		
		char http_port [10] = "80";
		char* port_str = strchr (host_str, ':');
	    	if (port_str != NULL) 
	    	{
			*port_str = '\0';
			port = port_str + 1;
		}
	    	else
	    		port = http_port;
	    		
	    	int destination_socket = client_creation (port, host_str);
		if (destination_socket == -1) 
	        {
			perror ("socket");
			close (client_socket);
			exit (EXIT_FAILURE);
	        }
		message_handler_http (cli_socket, destination_socket, data_buffer);
		
		close (destination_socket);
	       
	}


	/*int ser_socket = socket(AF_INET, SOCK_STREAM, 0);
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

	close(ser_socket);*/
}

