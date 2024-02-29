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
#include <sys/shm.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SIZE 3
#define MAX_CLIENTS 10

struct Player 
{
    int fd;
    int matched;
    char board[3][3];
};

int num_players = 0;
struct Player players[MAX_CLIENTS];

//char *board; // Pointer to the shared memory segment for the game board
//char board[3][3] = {{' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '}};
//int shmid;   // Shared memory ID
int client_sockets[MAX_CLIENTS];
int num_client = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for synchronization

void handle_websocket(int new_socket);
void handleMove(int k, int l) ;

// Initialize the game board in shared memory
void initializeBoard(int k) 
{
    pthread_mutex_lock(&mutex); // Lock mutex
    //memset(board, ' ', SIZE * SIZE);
    for (int i = 0; i < SIZE; i++) 
    {
        for (int j = 0; j < SIZE; j++) 
        {
            players[k].board[i][j] = ' ';
            
        }
    }
    
    printf("###");
    
    pthread_mutex_unlock(&mutex); // Unlock mutex
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


void handle_playreq(int id)
{
	
	int i,flag = 0;

	
	if(!players[id].matched){
		if (num_players >= 2) 
		{
			for(i=0;i<num_players;i++)
			{
				if(id!=i && !players[i].matched)
				{
					players[id].matched = 1;
		                       players[i].matched = 1;
		                      // matched_count +=2;
		                       printf("id : %d\ni : %d\n", id,i);
		                        	
		                       initializeBoard(i);
		                       initializeBoard(id);
		                       
		                       flag = 1;
		                       break;
		                        	
				}
			}
		}
        }
       
       
       
       if(flag)
       {
               handleMove(i,id);
	       players[i].matched = 0;
	       players[id].matched = 0;
       }	
}      


int parseWebSocketFrame(char *buffer, int len, char *output) 
{
    if (len < 6) 
    	return -1; // Insufficient data for header

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
    //printf("Frame: %s\n",frame);

    // Send the frame to the client
    send(new_socket, frame, len + 6, 0);
}

int isGameOver(int k) 
{
    // Check rows
    for (int i = 0; i < SIZE; i++) 
    {
        if (players[k].board[i][0] == players[k].board[i][1] && players[k].board[i][1] == players[k].board[i][2] && players[k].board[i][0] != ' ')
            return 1;
    }

    // Check columns
    for (int i = 0; i < SIZE; i++) 
    {
        if (players[k].board[0][i] == players[k].board[1][i] && players[k].board[1][i] == players[k].board[2][i] && players[k].board[0][i] != ' ')
            return 1;
    }

    // Check diagonals
    if ((players[k].board[0][0] == players[k].board[1][1] && players[k].board[1][1] == players[k].board[2][2] && players[k].board[0][0] != ' ') ||
        (players[k].board[0][2] == players[k].board[1][1] && players[k].board[1][1] == players[k].board[2][0] && players[k].board[0][2] != ' '))
        return 1;

    // Check if the board is full (draw)
    for (int i = 0; i < SIZE; i++) 
    {
        for (int j = 0; j < SIZE; j++) 
        {
            if (players[k].board[i][j] == ' ')
                return 0;
        }
    }

    return -1; // Draw
}

void handleMove(int k, int l) 
{

	    int currentPlayer = 1;
	    while(1)
	    {
	    	    int new_socket;
		    if(currentPlayer == 1) 
		    	new_socket = players[k].fd ;
		    else 
		    	new_socket = players[l].fd ;
		    
		    //printf("222 i : %d\n j : %d\n", k,l);
		    
		    char buffer[1096];
		    char message[BUFFER_SIZE];
		    int len = recv(new_socket, buffer, BUFFER_SIZE, 0);
		    //printf ("^^^\n");
		    if (len <= 0) 
		    {
		    	perror("Receive failed");
		    	exit(EXIT_FAILURE);
		    }
		    
		    // Parse WebSocket frame and extract payload
		    int payload_len = parseWebSocketFrame(buffer, len, message);
		    if (payload_len < 0) 
		    {
		    	perror("Invalid WebSocket frame");
		    	exit(EXIT_FAILURE);
		    }
		    
		    // Extract coordinates from the message
		    printf ("Received Message : %s\n", message);
		    
		    int row = message[0] - '0';
		    int col = message[2] - '0';
		    printf("R: %d, C: %d\n", row, col);
		    
		    int x=row;
		    int y=col;
		    
		    pthread_mutex_lock(&mutex); // Lock mutex
		    
		    
		    
		    char message4[BUFFER_SIZE];
		    char message2[BUFFER_SIZE];
		    char message3[BUFFER_SIZE];
		    int i;
		    if (players[k].board[x][y] == ' ') 
		    {
			players[k].board[x][y] = (currentPlayer == 1) ? 'X' : 'O';
			
			//printboard(k);
			
			sprintf(message4, "%d,%d,%c", x, y, players[k].board[x][y]);
			
			printf ("Mess: %s\n", message4);
			
			int i;
			for(i=0;i<=num_players;i++)
			{
				if(i==k || i==l)
					encodeAndSend(message4, strlen(message4),players[i].fd);
			}
				
			/*if (send(new_socket, message, strlen(message), 0) < 0)
				printf ("###\n");*/
			
			
			int result = isGameOver(k);
			
			printf("Current Player : %d\n",currentPlayer);
			
			printf ("Res: %d\n", result);
			if (result == 1) 
			{
			    printf("Player %d wins!\n", currentPlayer);
			    sprintf(message2, "Player %d wins!", currentPlayer);
			    strcpy(message3, "Game Over");
			    for(i=0;i<num_players;i++)
			    {
			    	if(i==k || i==l)
			    	{
					encodeAndSend(message2, strlen(message2), players[i].fd);
					encodeAndSend(message3, strlen(message3), players[i].fd);
	
				}
				
			    }
			    pthread_mutex_unlock(&mutex); // Unlock mutex
			    break;
			} 
			else if (result == -1) 
			{
			    printf("It's a draw!\n");
			    strcpy(message2, "It's a draw!");
			    for(i=0;i<num_players;i++)
			    {
			    	if(i==k || i==l)
			    	{
					encodeAndSend(message2, strlen(message2), players[i].fd);
					encodeAndSend(message3, strlen(message3), players[i].fd);
					
				}
				
			    }
			    pthread_mutex_unlock(&mutex); // Unlock mutex
			    break;
			} 
			else 
			{
			    strcpy(message2, "Next move");	
			    currentPlayer = (currentPlayer == 1) ? 2 : 1;
			}
			for(i=0;i<=num_client;i++)
			{
				if(i==k || i==l)
					encodeAndSend(message2, strlen(message2), players[i].fd);
			}
		    }
		    pthread_mutex_unlock(&mutex); // Unlock mutex
	    }
}

void *handleClient(void *arg) 
{
    //static int count = 0;
    int new_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int i;
    
    if (num_client < MAX_CLIENTS) 
    {
        client_sockets[num_client++] = new_socket;
    } 
    else 
    {
        printf("Max clients reached. Rejecting new connection.\n");
        close(new_socket);
        pthread_exit(NULL);
    }
    
 
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
    while (*key == ' ' || *key == ':') 
    	key++; // Skip spaces and colon
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
    
    handle_websocket(new_socket);
}
    
    
void handle_websocket(int new_socket)
{
    char buffer[1096];
    // Handle WebSocket communication with the client...
    int id = num_players;
    players[num_players].fd = new_socket;
    players[num_players].matched = 0;
    num_players++;
    
    while(1)
    {
		    
	    char message[BUFFER_SIZE];

	    // Main game loop
	    // Receive message from the client
	    int len = recv(new_socket, buffer, BUFFER_SIZE, 0);
	    //printf ("&&&\n");
	    if (len <= 0) 
	    {
		perror("Receive failed");
		exit(EXIT_FAILURE);
	    }
	    
	    // Parse WebSocket frame and extract payload
	    int payload_len = parseWebSocketFrame(buffer, len, message);
	    if (payload_len < 0) 
	    {
		perror("Invalid WebSocket frame");
		exit(EXIT_FAILURE);
	    }
	    
	    // Extract coordinates from the message
	   printf ("Received Message : %s \n", message); 
	    
	    //int i,j, flag = 0;
	    
	    if(strcmp(message, "PlayRequest")==0)
	    {
	    	
	    	while (1)
	    	{
	    		if(players[id].matched != 0)
	    		{
	    			break;
	    		}
			handle_playreq (id);
		}	
			
	    }
	    while(players[id].matched != 0); 
	    
    }
    
    for (int i = 0; i < num_client; i++) 
    {
        close(client_sockets[i]);
    }
    
    //close(server_fd);
    pthread_exit(NULL);
}



int main() 
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Create shared memory segment for the game board
    /*shmid = shmget(IPC_PRIVATE, SIZE * SIZE, IPC_CREAT | 0666);
    if (shmid == -1) 
    {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment
    board = (char *)shmat(shmid, NULL, 0);
    if (board == (void *)-1) 
    {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }*/

    // Initialize the game board
    //initializeBoard();
    
    int server_fd, new_socket;
    
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
    if (listen(server_fd, 3) < 0) 
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started. Listening on port %d...\n", PORT);
    
    while (1) 
    {
        // Accept incoming connections and handle them
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) 
        {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        // Handle each client in a separate thread
        pthread_t tid;
        if (pthread_create(&tid, NULL, handleClient, &new_socket) != 0) 
        {
            perror("pthread_create failed");
            close(new_socket);
        }
        
        if(pthread_detach(tid) != 0)
		perror("pthread_detach");
    }
    //shmdt(board);
    //shmctl(shmid, IPC_RMID, NULL);

    return 0;
}


