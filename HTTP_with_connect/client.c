#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void send_connect_request(const char *proxy_host, int proxy_port, const char *target_host, int target_port)
{
    int proxy_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in proxy_addr;
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_port = htons(proxy_port);
    if (inet_pton(AF_INET, proxy_host, &proxy_addr.sin_addr) <= 0)
    {
        perror("Invalid proxy address");
        exit(EXIT_FAILURE);
    }

    if (connect(proxy_socket, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) == -1)
    {
        perror("Connection to proxy failed");
        close(proxy_socket);
        exit(EXIT_FAILURE);
    }

    char connect_request[256];
    snprintf(connect_request, sizeof(connect_request), "CONNECT %s:%d HTTP/1.1\r\nHost: %s\r\n\r\n", target_host, target_port, target_host);

    if (send(proxy_socket, connect_request, strlen(connect_request), 0) == -1) 
    {
        perror("Failed to send CONNECT request");
        close(proxy_socket);
        exit(EXIT_FAILURE);
    }

    char response[4096];
    ssize_t bytes_received = recv(proxy_socket, response, sizeof(response) - 1, 0);
    if (bytes_received == -1)
    {
        perror("Failed to receive response");
        close(proxy_socket);
        exit(EXIT_FAILURE);
    }

    response[bytes_received] = '\0';
    printf("Proxy Response:\n%s\n", response);

    close(proxy_socket);
}


int main()
{
    const char *proxy_host = "127.0.0.1";
    int proxy_port = 8080;
    const char *target_host = "127.0.0.1";
    int target_port = 8888;

    send_connect_request(proxy_host, proxy_port, target_host, target_port);

    return 0;
}

