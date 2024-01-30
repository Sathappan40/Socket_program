#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define TARGET_PORT "8888"
#define SERVER_IP "127.0.0.1"

SSL_CTX *create_ssl_context() {
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_server_method());
    if (!ctx) {
        perror("Error creating SSL context");
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        perror("Error loading SSL certificate");
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        perror("Error loading SSL private key");
        exit(EXIT_FAILURE);
    }

    return ctx;
}

SSL *setup_ssl(SSL_CTX *ctx, int client_fd) {
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        perror("Error creating SSL object");
        exit(EXIT_FAILURE);
    }

    SSL_set_fd(ssl, client_fd);
    if (SSL_accept(ssl) <= 0) {
        perror("Error accepting SSL connection");
        exit(EXIT_FAILURE);
    }

    return ssl;
}

int main() {
    int target_fd, proxy_fd;
    struct sockaddr_in target_addr;
    SSL_CTX *ssl_ctx;
    SSL *ssl;

/*    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    ssl_ctx = create_ssl_context();*/

    target_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (target_fd == -1) {
        perror("Error creating target server socket");
        exit(EXIT_FAILURE);
    }

    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_addr.s_addr = INADDR_ANY;
    target_addr.sin_port = htons(atoi(TARGET_PORT));

    if (bind(target_fd, (struct sockaddr *)&target_addr, sizeof(target_addr)) == -1) {
        perror("Error binding");
        close(target_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening\n");

    if (listen(target_fd, 1) == -1) {
        perror("Error in listening");
        close(target_fd);
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %s\n", TARGET_PORT);

    while (1) {
        proxy_fd = accept(target_fd, NULL, NULL);
        if (proxy_fd == -1) {
            perror("Error accepting connection");
            close(target_fd);
            exit(EXIT_FAILURE);
        }

        printf("Connection accepted\n");
SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    ssl_ctx = create_ssl_context();

        ssl = setup_ssl(ssl_ctx, proxy_fd);

        char buffer[4096];
        ssize_t recv_bytes = SSL_read(ssl, buffer, sizeof(buffer));
        /*if (recv_bytes <= 0) {
            perror("Error receiving from client");
            close(proxy_fd);
            continue;
        }*/
        
        int sent_bytes;
        if (strstr(buffer, "CT")) {
            char *response = "<html><body><h1>Hello, this is the server response for CONNECT request!</h1></body></html>\r\n";
            sent_bytes = SSL_write(ssl, response, strlen(response));
            }
        if (strstr(buffer, "GET")) {
            char *response = "<html><body><h1>Hello, this is the server response for GET request!</h1></body></html>\r\n";
            sent_bytes = SSL_write(ssl, response, strlen(response));
            }
            if (sent_bytes <= 0) {
                perror("Error sending response to client");
                close(proxy_fd);
                continue;
            }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(proxy_fd);
       }

    SSL_CTX_free(ssl_ctx);
    close(target_fd);
    return 0;
}
