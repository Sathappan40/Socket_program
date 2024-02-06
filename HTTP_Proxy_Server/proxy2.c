#include <string.h>  
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <ctype.h>

#define PROXY_PORT 8888
#define BUFFER_SIZE 4096

void *get_in_addr (struct sockaddr *sa)
{
	if (sa -> sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa) -> sin_addr);	
	return &(((struct sockaddr_in6*)sa) -> sin6_addr);
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
	
	
	freeaddrinfo (servinfo);
	
	return sockfd;
}

char* extract_hostname(const char* request) {
    // Find the start of the URL in the request
    const char* url_start = strchr(request, ' ') + 1;
    if (url_start == NULL) {
        fprintf(stderr, "Invalid request format\n");
        return NULL;
    }

    // Find the end of the URL in the request
    const char* url_end = strchr(url_start, ' ');
    if (url_end == NULL) {
        fprintf(stderr, "Invalid request format\n");
        return NULL;
    }

    // Calculate the length of the URL
    size_t url_length = url_end - url_start;

    // Allocate memory for the URL and copy it
    char* url = (char*)malloc(url_length + 1);
    if (url == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strncpy(url, url_start, url_length);
    url[url_length] = '\0';

    // Find the start of the hostname in the URL
    const char* hostname_start = strstr(url, "://");
    if (hostname_start != NULL) {
        hostname_start += 3; // Move past "://"
    } else {
        hostname_start = url;
    }

    // Find the end of the hostname in the URL
    const char* hostname_end = strchr(hostname_start, '/');
    if (hostname_end == NULL) {
        hostname_end = url + url_length;
    }

    // Calculate the length of the hostname
    size_t hostname_length = hostname_end - hostname_start;

    // Allocate memory for the hostname and copy it
    char* hostname = (char*)malloc(hostname_length + 1);
    if (hostname == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strncpy(hostname, hostname_start, hostname_length);
    hostname[hostname_length] = '\0';

    // Free memory allocated for the URL
    free(url);

    return hostname;
}


// Forward the data between client and destination in an HTTPS connection
void message_handler (int client_socket, int destination_socket,char buffer1[])
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
			if (pollfds [0].revents & POLLIN)
			{
				n = read (pollfds [0].fd, buffer1, 1024);
				if (n <= 0)
					return;

				buffer1 [n] = '\0';

				write (pollfds [1].fd, buffer1, n);
			}
			
			//Message from server to client
			if (pollfds [1].revents & POLLIN)
			{
				n = read (pollfds [1].fd, buffer1, 1024);
				if (n <= 0)
					return;

				buffer1 [n] = '\0';

				write (pollfds [0].fd, buffer1, n);
			}
		}
	}
}

void message_handler_http (int client_socket, int destination_socket,char buffer1[])
{
	ssize_t n;
	
	n = write (destination_socket, buffer1, 2048);
	
	while ((n = read(destination_socket, buffer1, 2048)) > 0)
		write(client_socket, buffer1, n);
}



void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int server_socket;
    ssize_t bytes_read, bytes_written;

    // Read the CONNECT request from the client
    bytes_read = read(client_socket, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
        perror("Failed to read from client");
        close(client_socket);
        return;
    }
    printf("%s",buffer);

    // Extract the hostname and port from the CONNECT request
    /*char *hostname = strtok(buffer + 8, ":");
    char *port_str = strtok(NULL, "\r\n");
    int port = atoi(port_str);*/
    
    char method[16];
    char host[256];
    char buffer1[4096];
    strcpy(buffer1, buffer);
    printf ("%s\n\n", buffer1);
    sscanf (buffer, "%s %s", method, host);
    
    if(strcmp(method, "CONNECT") == 0)
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
	    
	    char *hostnm = strtok(buffer1+8, ":");
	    
	    server_socket = client_creation (port, host);
	    printf ("P:%s\n\n", port);
    }
    
    else
    {
    	char *host_str = strstr (buffer, "Host: ") + 6;
	char *host_end = strstr (host_str, "\r\n");
	//*host_end = '\0';	
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
    		
    	char* host = extract_hostname(buffer);
    		
    	server_socket = client_creation (port, host);
    	printf ("PORT:%s\n\n", port);
    	printf("%s\n\n", host);
    
    }
    
    printf ("%s\n\n", method);
    printf ("%s\n\n", host);
    
    //printf ("%s\n\n", hostnm);
    
    /*server_socket = client_creation (port, host);
    if (server_socket == -1) 
    {
    	perror ("socket");
    	close (client_socket);
    	exit (EXIT_FAILURE);
    }
    */
    /*struct hostent *host_info;
    struct in_addr **addr_list;

    // Resolve the hostname
    host_info = gethostbyname(hostnm);
    if (host_info == NULL) {
        perror("gethostbyname");
        exit(EXIT_FAILURE);
    }
    
     // Extract the list of IP addresses
    addr_list = (struct in_addr **)host_info->h_addr_list;

    if (addr_list[0] == NULL) {
        fprintf(stderr, "No IP address found for %s\n", hostnm);
        exit(EXIT_FAILURE);
    }

    // Convert the IP address string to binary representation
    in_addr_t ip_binary = inet_addr(inet_ntoa(*addr_list[0]));
    if (ip_binary == INADDR_NONE) {
        fprintf(stderr, "Invalid IP address\n");
        exit(EXIT_FAILURE);
    }
    printf("IPv4 address in binary (network byte order): %08x\n", ip_binary);

		

    // Create a TCP connection to the server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    //char* str = ip_of_server(port,host);
    server_addr.sin_addr.s_addr = ip_binary;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create server socket");
        close(client_socket);
        return;
    }
    
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server");
        close(client_socket);
        close(server_socket);
        return;
    }*/
    
    //server_socket = client_creation (port, host);
    
    printf("hello");

    // Send a response to the client indicating that the connection is established
    const char *response = "HTTP/1.1 200 Connection established\r\n\r\n";
    bytes_written = write(client_socket, response, strlen(response));
    if (bytes_written <= 0) {
        perror("Failed to write response to client");
        close(client_socket);
        close(server_socket);
        return;
    }

    // Forward data between client and server
    //while ((bytes_read = read(client_socket, buffer, sizeof(buffer))) > 0) 
    /*while(1){
        bytes_written = write(server_socket, buffer, bytes_read);
        if (bytes_written <= 0) {
            perror("Failed to write to server");
            break;
        }
        bytes_read = read(server_socket, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            perror("Failed to read from server");
            break;
        }
        bytes_written = write(client_socket, buffer, bytes_read);
        if (bytes_written <= 0) {
            perror("Failed to write to client");
            break;
        }
    }*/
    
    if(strcmp(method, "CONNECT") == 0)
    {
    	message_handler(client_socket,server_socket,buffer1);
    }
    else
    {
    	message_handler_http (client_socket, server_socket,buffer1);
    }
    
    close(client_socket);
    close(server_socket);
}

int main() {
    int proxy_socket, client_socket;
    struct sockaddr_in proxy_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create proxy socket
    proxy_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_socket < 0) {
        perror("Failed to create proxy socket");
        exit(EXIT_FAILURE);
    }

    // Set proxy socket options
    int optval = 1;
    setsockopt(proxy_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // Bind proxy socket
    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_addr.sin_port = htons(PROXY_PORT);
    if (bind(proxy_socket, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0) {
        perror("Failed to bind proxy socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(proxy_socket, 5) < 0) {
        perror("Failed to listen on proxy socket");
        exit(EXIT_FAILURE);
    }

    printf("Proxy server started on port %d\n", PROXY_PORT);

    // Accept and handle incoming connections
    while (1) {
        client_socket = accept(proxy_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Failed to accept connection");
            continue;
        }

        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        handle_client(client_socket);
    }

    return 0;
}
