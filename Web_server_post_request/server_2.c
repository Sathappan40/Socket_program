#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define FILENAME_MAX_LENGTH 256

void handle_post_request(int client_fd, char *request) {
    char *boundary_start = strstr(request, "boundary=");
    if (!boundary_start) {
        // Invalid request format
        send(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", 29, 0);
        return;
    }
    

    boundary_start += strlen("boundary=");
    char *boundary_end = strchr(boundary_start, '\r');
    if (!boundary_end) {
        // Invalid boundary format
        send(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", 29, 0);
        return;
    }
    //printf("Boundary Start : %s\n\n\n\n", boundary_start);
    //printf("Boundary End : %s\n\n\n\n", boundary_end);

    // Extract boundary string
    char boundary[128];
    strncpy(boundary, boundary_start, boundary_end - boundary_start);
    boundary[boundary_end - boundary_start] = '\0';

    // Find start of first part
    char *part_start = strstr(boundary_end, "\r\n\r\n") + 4;
    if (!part_start) {
        // Invalid request format
        send(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", 29, 0);
        return;
    }

    // Find end of first part
    char *part_end = strstr(part_start, boundary);
    if (!part_end) {
        // Invalid request format
        send(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", 29, 0);
        return;
    }
    //printf("Part Start : %s\n\n\n\n", part_start);
    //printf("Part End : %s\n\n\n\n", part_end);
    
    int count = 0;
    
    char filename[FILENAME_MAX_LENGTH];
    snprintf(filename, FILENAME_MAX_LENGTH, "data_in_post%ld.txt", time(NULL)); // Generate unique filename
    FILE *file = fopen(filename, "w");
    if (!file) {
        // Error opening file
        send(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n", 39, 0);
        return;
    }

    // Process each part
    //while (part_start < part_end) 
    while(count<2)
    {
        // Find content disposition
        char *disposition_start = strstr(part_start, "Content-Disposition:");
        if (!disposition_start) {
            // Invalid part format
            send(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", 29, 0);
            fclose(file);
            return;
        }

        // Extract field name
        char *name_start = strstr(disposition_start, "name=\"") + strlen("name=\"");
        char *name_end = strchr(name_start, '"');
        if (!name_end) {
            // Invalid part format
            send(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", 29, 0);
            return;
        }
        //printf("Name Start : %s\n\n\n\n", name_start);
        //printf("Name End : %s\n\n\n\n", name_end);

        char field_name[128];
        strncpy(field_name, name_start, name_end - name_start);
        field_name[name_end - name_start] = '\0';

        // Find start of part data
        char *data_start = strstr(disposition_start, "\r\n\r\n") + 4;
        if (!data_start) {
            // Invalid part format
            send(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", 29, 0);
            fclose(file);
            return;
        }

        // Find end of part data
        char *data_end = strstr(data_start, boundary);
        if (!data_end) {
            // Invalid part format
            send(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", 29, 0);
            fclose(file);
            return;
        }
        //printf("Data Start: %s\n\n\n\n", data_start);
        //printf("Data End : %s\n\n\n\n", data_end);

        // Print field name and data to terminal
        printf("Field: %s , Data: %.*s\n", field_name, (int)(data_end - data_start), data_start);
        
        fwrite(data_start, sizeof(char), data_end - data_start, file);

        // Move to next part
        part_start = data_end + strlen(boundary);
    }
    
    fclose(file);

    // Send response
    send(client_fd, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);
    count++;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
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
        printf("\n\nBUFFER : %s\n\n\n\n", buffer);

        // Handle POST request
        handle_post_request(client_fd, buffer);

        // Close client socket
        close(client_fd);
    }

    return 0;
}

