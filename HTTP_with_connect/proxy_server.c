#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PROXY_PORT "8080" 
#define SERVER_IP "127.0.0.1"
#define BACKLOG 10
#define TARGET_HOST "localhost"
#define TARGET_PORT "8888" 

SSL_CTX *create_ssl_context()
{
	SSL_CTX *ctx;
	
	SSL_library_init();
	ctx = SSL_CTX_new(SSLv23_server_method());
	
	if(!ctx)
	{
		ERR_print_errors_fp(stderr);
		exit(1);
	}
	
	return ctx;
}


SSL *setup_ssl(SSL_CTX *ctx, int client_fd)
{
	SSL *ssl = SSL_new(ctx);
	if(!ssl)
	{
		ERR_print_errors_fp(stderr);
		exit(1);
	}
	
	SSL_set_fd(ssl, client_fd);
	
	if(SSL_accept(ssl) <= 0)
	{
		ERR_print_errors_fp(stderr);
		exit(1);
	}
	
	return ssl;
}

	  

int main()
{
	int proxy_fd, client_fd;
	struct sockaddr_storage cli_addr;
	socklen_t sin_size;
	SSL_CTX *ssl_ctx;
	SSL *ssl;
	
	ssl_ctx = create_ssl_context();
	
	proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(proxy_fd == -1)
	{
		perror("Error in proxy server socket setup");
		exit(1);
	}
	
	struct sockaddr_in proxy_addr;
	memset(&proxy_addr, 0, sizeof(proxy_addr));
	proxy_addr.sin_family = AF_INET;
	proxy_addr.sin_addr.s_addr = INADDR_ANY;
	proxy_addr.sin_port = htons(atoi(PROXY_PORT));
	
	if(bind(proxy_fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) == -1)
	{
		perror("Error in binding in proxy server");
		close(proxy_fd);
		exit(1);
	}
	
	if(listen(proxy_fd, BACKLOG) == -1)
	{
		perror("Error in listening");
		close(proxy_fd);
		exit(1);
	}
	
	printf("Proxy server : Waiting for connection");
	
	while(1)
	{
		sin_size = sizeof(cli_addr);
		client_fd = accept(proxy_fd, (struct sockaddr*)&cli_addr, &sin_size);
		if(client_fd == -1)
		{
			perror("Error in accepting");
			continue;
		}
		
		ssl = setup_ssl(ssl_ctx, client_fd);
		
		char buffer[4096];
		//ssize_t received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
		ssize_t recv1 = recv(client_fd, buffer, sizeof(buffer), 0);

		
		if(strstr(buffer, "CONNECT") != NULL)
		{
			//handle_connect_request(ssl);
			
			/*const char *message = "Connect request processed by the proxy";
			SSL_write(ssl, message, strlen(message));*/
			
			char *resp = "HTTP/1.1 200 connection establised\r\n\r\n";
			ssize_t send1 = send(client_fd, resp, strlen(resp), 0);
			
			int target_fd = socket(AF_INET, SOCK_STREAM, 0);
			if(target_fd == -1)
			{
				perror("Error creating target server sockt");
				SSL_shutdown(ssl);
				SSL_free(ssl);
				close(client_fd);
				continue;
			}
			
			struct sockaddr_in target_addr;
			memset(&target_addr, 0, sizeof(target_addr));
			target_addr.sin_family = AF_INET;
			target_addr.sin_port = htons(atoi(TARGET_PORT));
			target_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
			
		        /*struct hostnet *target_hostent = gethostbyname(TARGET_HOST);
		        if(target_hostent == NULL)
		        {
		        	perror("Error in getting hostbyname");
		        	close(target_fd);
		        	SSL_shutdown(ssl);
		        	SSL_free(ssl);
		        	close(client_fd);
		        	continue;
		        }*/
		        
		        //memcpy(&target_addr.sin_addr, target_hostent->h_addr_list[0], target_hostent->h_length);
		        
		        if(connect(target_fd, (struct sockaddr*)&target_addr, sizeof(target_addr)) == -1)
		        {
		        	perror("Error in connecting target server");
		        	close(target_fd);
		        	SSL_shutdown(ssl);
		        	SSL_free(ssl);
		        	close(client_fd);
		        	continue;
		        }
		        
		        SSL *target_ssl = setup_ssl(ssl_ctx, target_fd);
		        
		        while(1)
		        {
		        	fd_set read_fds;
		        	FD_ZERO(&read_fds);
		        	FD_SET(client_fd, &read_fds);
		        	FD_SET(target_fd, &read_fds);
		        	
		        	int max_fd = (client_fd > target_fd) ? client_fd : target_fd;
		        	if(select(max_fd+1, &read_fds, NULL, NULL, NULL) == -1)
		        	{
		        		perror("select error");
		        		break;
		        	}
		        	
		        	if(FD_ISSET(client_fd, &read_fds))
		        	{
		        		ssize_t received2 = SSL_read(ssl, buffer, sizeof(buffer));
		        		if(received2 <= 0)
		        		{
		        			perror("Error in receiving from client");
		        			break;
		        		}
		        		
		        		SSL_write(target_ssl, buffer, received2);
		        	}
		        	
		        	if(FD_ISSET(target_fd, &read_fds))
		        	{
		        		ssize_t received3 = SSL_read(target_ssl, buffer, sizeof(buffer));
		        		if(received3 < 0)
		        		{
		        			perror("Error in receiving from server");
		        			break;
		        		}
		        		SSL_write(ssl, buffer, received3);
		        	}
		        }
		        
		        SSL_shutdown(target_ssl);
		        SSL_free(target_ssl);
		        
		        close(target_fd);
		}
		
		SSL_shutdown(ssl);
		SSL_free(ssl);
		close(client_fd);
	}
	
	SSL_CTX_free(ssl_ctx);
	return 0;
}

/*void handle_connect_request(SSL *ssl)
{
	char response[] = "HTTP/1.1 200 connection establised\r\n\r\n";
	//SSL_write(ssl, response, sizeof(response));
	ssize_t s = send(client_fd, response, sizeof(response), 0);
}*/

		        	
			
