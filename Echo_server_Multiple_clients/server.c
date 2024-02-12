#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096

int main() {
    int master_socket, addrlen, new_socket, client_socket[MAX_CLIENTS], 
        activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address, s_address;

    char buffer[BUFFER_SIZE];

    fd_set readfds;

    // Create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set master socket to allow multiple connections
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, 
        (char *)&(int){1}, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    s_address.sin_family = AF_INET;
    s_address.sin_addr.s_addr = INADDR_ANY;
    s_address.sin_port = htons(8888);

    // Bind the socket to localhost port 8888
    if (bind(master_socket, (struct sockaddr *)&s_address, sizeof(s_address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(s_address);
    puts("Waiting for connections ...");

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        // Add child sockets to set
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];

            // If valid socket descriptor, add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            // Highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        // Wait for activity on any of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        // If something happened on the master socket, then it's an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            printf("New connection, socket fd is %d, ip is: %s, port : %d\n",
                    new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // Add new socket to array of sockets
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);
                    break;
                }
            }
        }

        // Else, it's some IO operation on some other socket
        for (i = 0; i < MAX_CLIENTS; i++) 
        {

            if (FD_ISSET(i, &readfds)) 
            {
              if(i != master_socket)
              {            
                // Check if it was for closing, and also read the incoming message
                if ((valread = read(i, buffer, BUFFER_SIZE)) <= 0) {
                    // Somebody disconnected, get their details and print
                    //getpeername(sd, (struct sockaddr*)&address, \
                        (socklen_t*)&addrlen);
                    /*printf("Host disconnected, ip %s, port %d\n", 
                          inet_ntoa(address.sin_addr), ntohs(address.sin_port));*/

                    // Close the socket and mark as 0 in list for reuse
                    perror("recv");
                    close(i);
                    client_socket[i] = 0;
                }

                // Echo back the message that came in
                else {
                    // Set the terminating NULL byte on the end of the data read
                    buffer[valread] = '\0';
                    send(i, buffer, strlen(buffer), 0);
                }
              }
            }
        }
    }

    return 0;
}

