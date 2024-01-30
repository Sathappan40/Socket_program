#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define TARGET_PORT "8888"
#define SERVER_IP "127.0.0.1"

SSL_CTX *create_ssl_context() 
{
    SSL_CTX *ctx;

    //SSL_library_init();
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
    /*if (!ssl) 
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }*/

    SSL_set_fd(ssl, client_fd);
    int flag;


    if (flag = SSL_accept(ssl)) 
    {
	SSL_free(ssl);
	SSL_CTX_free(ctx);
        printf("%d", flag);    	
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
        
        
    }

    return ssl;
}

int main()
{
	int target_fd, proxy_fd;
	struct sockaddr_in target_addr, proxy_addr;
	SSL_CTX *ssl_ctx;
	SSL *ssl;
/*	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();
	ssl_ctx = create_ssl_context();*/
	
	target_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(target_fd == -1)
	{
		perror("Error in target server socket");
		exit(1);
	}
	
	memset(&target_addr, 0, sizeof(target_addr));
	target_addr.sin_family = AF_INET;
	target_addr.sin_addr.s_addr = INADDR_ANY;
	target_addr.sin_port = htons(atoi(TARGET_PORT));
	
	if(bind(target_fd, (struct sockaddr *)&target_addr, sizeof(target_addr)) == -1)
	{
		perror("Error in binding");
		close(target_fd);
		exit(1);
	}
	
	printf("Server is listening");
	
	if(listen(target_fd, 1) == -1)
	{
		perror("Error in listening");
		close(target_fd);
		exit(1);
	}
	
	printf("  %d\n",target_fd);
	
	while(1){
	proxy_fd = accept(target_fd, NULL, NULL);
	

	if(proxy_fd == -1)
	{
		perror("Error in accepting");
		close(target_fd);
		exit(1);
	}
	printf("hello\n");	
	SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        OpenSSL_add_all_algorithms();
        ssl_ctx = create_ssl_context();

	ssl = setup_ssl(ssl_ctx, proxy_fd);
	
	printf("helloo\n");
	

	
		char buffer[4096];
		ssize_t recv;
		
		/*fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(proxy_fd, &read_fds);
		
		if(select(proxy_fd+1, &read_fds, NULL, NULL, NULL) == -1)
		{
			perror("select error");
			break;
		}*/
		
		recv = SSL_read(ssl, buffer, sizeof(buffer));
		if(recv <= 0)
		{
			perror("Error in receiving from proxy");
			break;
		}
		
	        if(strstr(buffer,"GET"))
		{
		char *response = "<html><body><h1>Hello, this is the server response for CONNECT request!</h1></body></html>\r\n";
		int sent = SSL_write(ssl, response, strlen(response));
		}	
		/*char *message = "Connect is processed";
		SSL_write(targe, buffer, received2);*/
		        	
	
	SSL_shutdown(ssl);
  	SSL_free(ssl);
        SSL_CTX_free(ssl_ctx);	
	

	}
   close(proxy_fd);
	return 0;
}

