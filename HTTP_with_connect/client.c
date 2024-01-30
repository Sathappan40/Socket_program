#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PROXY_SERVER "127.0.0.1"
#define PROXY_PORT 8080

int main() {
    int sockfd;
    struct sockaddr_in proxy_addr;

    // Set up socket to connect to the proxy
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket");
        exit(EXIT_FAILURE);
    }

    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = inet_addr(PROXY_SERVER);
    proxy_addr.sin_port = htons(PROXY_PORT);

    // Connect to the proxy server
    if (connect(sockfd, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr)) == -1) {
        perror("Connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Send CONNECT request
    const char *connect_request = "CONNECT 127.0.0.1:8888 HTTP/1.1\r\n\r\n";
    send(sockfd, connect_request, strlen(connect_request), 0);

    // Receive and print the response from the proxy
    char buffer[4096];
    ssize_t received;
    while ((received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[received] = '\0';
        printf("%s", buffer);
    }


    // Close the socket
    close(sockfd);

    return 0;
}

