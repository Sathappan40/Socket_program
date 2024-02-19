#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void write_to_file(const char *data) 
{
    FILE *file = fopen("post_data.txt", "a");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    fprintf(file, "%s\n", data);
    fclose(file);
}

void handle_request(int client_fd, char *request) 
{
    // Only handle POST requests
    if (strncmp(request, "POST", 4) != 0) 
    {
        char *response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Extract data from POST request
    char *data_start = strstr(request, "\r\n\r\n") + 4;
    if (!data_start) 
    {
        char *response = "HTTP/1.1 400 Bad Request\r\n\r\n";
        send(client_fd, response, strlen(response), 0);
        return;
    }

    // Write data to file
    write_to_file(data_start);

    // Send response
    char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>POST request received and data written to file</h1>";
    send(client_fd, response, strlen(response), 0);
}

int main() 
{
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) 
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) 
    {
        // Accept incoming connection
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("Connection accepted.\n");

        // Receive HTTP request
        int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }
        
        printf("%s", buffer);

        // Handle HTTP request
        handle_request(client_fd, buffer);

        // Close client socket
        close(client_fd);
    }

    return 0;
}

