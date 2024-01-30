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

void print_certificate(SSL *ssl) {
    X509 *cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL) {
        char *subject = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        char *issuer = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        
        printf("Subject: %s\n", subject);
        printf("Issuer: %s\n", issuer);
        
        X509_free(cert);
        free(subject);
        free(issuer);
    } else {
        printf("No certificate available\n");
    }
}

SSL_CTX *create_ssl_context1() {
	//SSL_library_init();
        SSL_load_error_strings();
        SSL_CTX *ctx1;

        ctx1 = SSL_CTX_new(SSLv23_client_method());

        if(ctx1 == NULL) {
                ERR_print_errors_fp(stderr);
                exit(1);
        }
        return ctx1;
}


SSL_CTX *create_ssl_context()
{
	/*SSL_CTX *ctx;
	
	SSL_library_init();
	ctx = SSL_CTX_new(SSLv23_server_method());
	
	if(!ctx)
	{
		ERR_print_errors_fp(stderr);
		exit(1);
	}
	
	return ctx;*/
	
	SSL_CTX *ctx;

   // SSL_library_init();
    ctx = SSL_CTX_new(SSLv23_server_method());

    if (!ctx) 
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0)  {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) 
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}


SSL *setup_ssl(SSL_CTX *ctx, int client_fd)
{
	SSL *ssl = SSL_new(ctx);
	if(ssl <= 0)
	{
		printf ("(\n");
		ERR_print_errors_fp(stderr);
		exit(1);
	}
	
	SSL_set_fd(ssl, client_fd);
	
	
	int flag = SSL_accept(ssl);
	printf (" %d \n", flag);
	if(flag <= 0)
	{
		
		/*ERR_print_errors_fp(stderr);
		exit(1);*/
		
		 int ssl_error = SSL_get_error(ssl, flag);
    
        	switch (ssl_error) 
        	{
        	case SSL_ERROR_WANT_READ:
           	  	printf("SSL_accept error: SSL_ERROR_WANT_READ\n");
            		break;
            
        	case SSL_ERROR_WANT_WRITE:
            		printf("SSL_accept error: SSL_ERROR_WANT_WRITE\n");
            		break;

        	case SSL_ERROR_ZERO_RETURN:
            		printf("SSL_accept error: SSL_ERROR_ZERO_RETURN\n");
			break;

		case SSL_ERROR_SYSCALL:
            		    perror("SSL_accept");
			    printf("SSL_accept error: SSL_ERROR_SYSCALL\n");
			    break;

        	default:
            		fprintf(stderr, "SSL_accept error: %d\n", ssl_error);
		}
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
	
	/*SSL_library_init();
	ssl_ctx = create_ssl_context();*/
	
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
		printf ("@\n");
		close(proxy_fd);
		exit(1);
	}
	
	if(listen(proxy_fd, BACKLOG) == -1)
	{
		perror("Error in listening");
		printf ("#\n");
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
			printf ("+\n");
			continue;
		}
		
		printf ("^\n");
		SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        OpenSSL_add_all_algorithms();
        ssl_ctx = create_ssl_context1();
		ssl = setup_ssl(ssl_ctx, client_fd);
		printf ("&\n");
		
		char buffer[4096];
		//ssize_t received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
		ssize_t recv1 = recv(client_fd, buffer, sizeof(buffer), 0);
		printf ("Buff: %s\n", buffer);
		
		if(strstr(buffer, "CT") != NULL)
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
		        
		        
		        /*if(SSL_connect(ssl) == -1)
		        {
		        	perror("Error in connecting target server");
		        	close(client_fd);
		        	SSL_shutdown(ssl);
		        	SSL_free(ssl);
		        	
		        	continue;
		        }*/
		        printf("%d", target_fd);
		        
		        SSL_CTX *target_ctx=create_ssl_context();
		        SSL *target_ssl = setup_ssl(target_ctx, target_fd);
		        print_certificate(target_ssl);
		        
		        if(SSL_connect(target_ssl) == -1)
		        {
		        	perror("Error in connecting target server");
		        	close(client_fd);
		        	SSL_shutdown(ssl);
		        	SSL_free(ssl);
		        	
		        	continue;
		        }
		    
		        	/*fd_set read_fds;
		        	FD_ZERO(&read_fds);
		        	FD_SET(client_fd, &read_fds);
		        	FD_SET(target_fd, &read_fds);
		        	
		        	int max_fd = (client_fd > target_fd) ? client_fd : target_fd;
		        	if(select(max_fd+1, &read_fds, NULL, NULL, NULL) == -1)
		        	{
		        		perror("select error");
		        		break;
		        	}*/
		        	
		        	/*ssize_t received2 = SSL_read(ssl, buffer, sizeof(buffer));
		        	if(received2 <= 0)
		        	{
		        		perror("Error in receiving from client");
		        		break;
		        	}
		        		
		        	SSL_write(target_ssl, buffer, received2);*/
		        	
		        	printf("%d", target_fd);
		        	SSL_write(target_ssl, buffer, recv1);
		        	
		   
		        	ssize_t received3 = SSL_read(target_ssl, buffer, sizeof(buffer));
		        	if(received3 < 0)
		        	{
		        		perror("Error in receiving from server");
		        		break;
		        	}
		        	SSL_write(ssl, buffer, received3);
		        	
		        
		        
		        SSL_shutdown(target_ssl);
		        SSL_free(target_ssl);
		        SSL_CTX_free(target_ctx);
		        
		        close(target_fd);
		}
		
		SSL_shutdown(ssl);
		SSL_free(ssl);
		close(client_fd);
	
	
	SSL_CTX_free(ssl_ctx);}
	
	
	return 0;
}

/*void handle_connect_request(SSL *ssl)
{
	char response[] = "HTTP/1.1 200 connection establised\r\n\r\n";
	//SSL_write(ssl, response, sizeof(response));
	ssize_t s = send(client_fd, response, sizeof(response), 0);
}*/

		        	
			
