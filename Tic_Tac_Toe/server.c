#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#define PORT 8080
#define BUFFER_SIZE 1024

#define SIZE 3
int currentPlayer = 1;	
char board[3][3] = {{' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '}};

void encodeAndSend(const char *message, int len, int new_socket) 
{
    // Construct WebSocket frame
    char frame[BUFFER_SIZE];
    frame[0] = 0x81; // Fin bit set, opcode for text data
    frame[1] = 0x80 | len; // Mask bit set, payload length

    // Generate random mask key
    unsigned char mask_key[4];
    for (int i = 0; i < 4; i++) 
    {
        mask_key[i] = rand() % 256;
    }

    // Copy mask key into frame
    memcpy(frame + 2, mask_key, 4);

    // Mask the payload
    for (int i = 0; i < len; i++) 
    {
        frame[i + 6] = message[i] ^ mask_key[i % 4];
    }
    printf("Frame: %s\n",frame);

    // Send the frame to the client
    send(new_socket, frame, len + 6, 0);
}

int isGameOver() 
{
    // Check rows
    for (int i = 0; i < SIZE; i++) 
    {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return 1;
    }

    // Check columns
    for (int i = 0; i < SIZE; i++) 
    {
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return 1;
    }

    // Check diagonals
    if ((board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ') ||
        (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' '))
        return 1;

    // Check if the board is full (draw)
    for (int i = 0; i < SIZE; i++) 
    {
        for (int j = 0; j < SIZE; j++) 
        {
            if (board[i][j] == ' ')
                return 0;
        }
    }

    return -1; // Draw
}


void handleMove(int x, int y, int new_socket) 
{
    char message[BUFFER_SIZE];
    char message2[BUFFER_SIZE];
    if (board[x][y] == ' ') 
    {
        board[x][y] = (currentPlayer == 1) ? 'X' : 'O';
        sprintf(message, "%d,%d,%c", x, y, board[x][y]);
        
        printf ("Mess: %s\n", message);
        encodeAndSend(message, strlen(message), new_socket);
        
        /*if (send(new_socket, message, strlen(message), 0) < 0)
        	printf ("###\n");*/
        
        
        int result = isGameOver();
        
        printf("Player : %d\n",currentPlayer);
        
        printf ("Res: %d\n", result);
        if (result == 1) 
        {
            printf("Player %d wins!\n", currentPlayer);
            sprintf(message2, "Player %d wins!", currentPlayer);
            //initializeBoard();
        } 
        else if (result == -1) 
        {
            printf("It's a draw!\n");
            strcpy(message2, "It's a draw!");
            //initializeBoard();
        } 
        else 
        {
            currentPlayer = (currentPlayer == 1) ? 2 : 1;
        }
        encodeAndSend(message2, strlen(message2), new_socket);
    }
}

int parseWebSocketFrame(char *buffer, int len, char *output) 
{
    if (len < 6) return -1; // Insufficient data for header

    // Check if it's a final frame and opcode is text (0x81)
    if ((buffer[0] & 0x80) == 0x80 && (buffer[0] & 0x0F) == 0x01) 
    {
        int payload_len = buffer[1] & 0x7F;
        int mask_offset = 2;
        int data_offset = mask_offset + 4;

        if (payload_len <= 125) 
        {
            // Extract payload
            for (int i = 0; i < payload_len; i++) 
            {
                output[i] = buffer[data_offset + i] ^ buffer[mask_offset + i % 4];
            }
            output[payload_len] = '\0'; // Null-terminate string
            return payload_len;
        }
    }
    return -1; // Invalid frame
}

unsigned char* sha1_hash(const char *input) 
{

    printf("\n%s\n",input);
    static unsigned char sha1_hash1[SHA_DIGEST_LENGTH];
    SHA1(( unsigned char *)input, strlen(input), sha1_hash1);
    printf("Hash : %s\n",sha1_hash1);

    return sha1_hash1;
}

char* base64_encode(const char *input) 
{  
    char *output = (char *)malloc(40);
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO *bio = BIO_new(BIO_s_mem());
    BIO_push(b64, bio);

    BIO_write(b64, input, SHA_DIGEST_LENGTH);
    BIO_flush(b64);

    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);
    
    strcpy(output, bptr->data);
    size_t len = strlen(output);
    if (len > 0 && output[len - 1] == '\n') {
        output[len - 1] = '\0';
    }

    BIO_free_all(b64);
    return output;
}

int main() 
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
    {
	 perror("Setsockopt failed");
	 exit(EXIT_FAILURE);
    }
	    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) 
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started. Listening on port %d...\n", PORT);

    while(1)
    {
	    // Accept incoming connections and handle them
	    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) 
	    {
		perror("Accept failed");
		exit(EXIT_FAILURE);
	    }

	    printf("Client connected to\n");
	    
	     if(fork() == 0)
	     { 
	            

		    // Receive WebSocket handshake request from the client
		    if (recv(new_socket, buffer, BUFFER_SIZE, 0) <= 0) 
		    {
			perror("Receive failed");
			exit(EXIT_FAILURE);
		    }

		    printf("Received handshake request:\n%s\n", buffer);

		    // Find WebSocket key from the request
		    char *key = strstr(buffer, "Sec-WebSocket-Key:");
		    if (key == NULL) 
		    {
			perror("WebSocket key not found");
			exit(EXIT_FAILURE);
		    }
		    key += strlen("Sec-WebSocket-Key:");
		    while (*key == ' ' || *key == ':') key++; // Skip spaces and colon
		    char *end = strchr(key, '\r');
		    if (end == NULL) 
		    {
			perror("Invalid WebSocket key");
			exit(EXIT_FAILURE);
		    }
		    *end = '\0'; // Null-terminate the key

		    // Concatenate WebSocket key with the magic string
		    char magic_string[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
		    char combined_key[strlen(key) + sizeof(magic_string)];
		    strcpy(combined_key, key);
		    strcat(combined_key, magic_string);

		    // Compute SHA-1 hash of the combined string
		    unsigned char *sha1_result = sha1_hash(combined_key);
		    printf("%s\n",sha1_result);

		    // Encode SHA-1 hash in Base64
		    char *base64_result = base64_encode(sha1_result);

		    // Prepare handshake response
		    char handshake_response[BUFFER_SIZE];
		    sprintf(handshake_response, "HTTP/1.1 101 Switching Protocols\r\n"
				                 "Upgrade: websocket\r\n"
				                 "Connection: Upgrade\r\n"
				                 "Sec-WebSocket-Accept: %s\r\n"
				                 "\r\n", base64_result);

		    // Send handshake response to the client
		    send(new_socket, handshake_response, strlen(handshake_response), 0);
		    printf("Sent handshake response:\n%s\n", handshake_response);

		    // Handle WebSocket communication with the client...
		    

		    char message[BUFFER_SIZE];

		    // Initialize the tic-tac-toe board

		    // Main game loop
			// Receive message from the client
			int len = recv(new_socket, buffer, BUFFER_SIZE, 0);
			if (len <= 0) 
			{
			    perror("Receive failed");
			    break;
			}

			// Parse WebSocket frame and extract payload
			int payload_len = parseWebSocketFrame(buffer, len, message);
			if (payload_len < 0) 
			{
			    perror("Invalid WebSocket frame");
			    break;
			}

			// Extract coordinates from the message
			printf ("Received Message : %s\n", message);
			int row = message[0] - '0';
			int col = message[2] - '0';
			
			printf ("R: %d, C: %d\n", row, col);
			// Check if the move is valid
			if (row < 0 || row >= 3 || col < 0 || col >= 3 || board[row][col] != ' ') 
			{
			    strcpy(message, "Invalid move");
			} 
			else
			{
			    handleMove(row, col, new_socket);
			    sprintf(message, "You placed 'X' at position (%d, %d)", row, col);
			    printf ("Message: %s\n", message);
			} 
			//send(new_socket, message, strlen(message), 0);
		    close(new_socket);
		    //close(server_fd);
	    }
    }

    return 0;
}

