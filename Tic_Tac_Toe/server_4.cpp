#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <pthread.h>
#include<thread>
#include <mutex>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SIZE 3
#define MAX_CLIENTS 10

using namespace std;

class Player 
{
public:
    int fd;
    int matched;
    int req;
};

class CryptoUtils 
{
public:
    static unsigned char* sha1_hash(const char *input) 
    {
        static unsigned char sha1_hash1[SHA_DIGEST_LENGTH];
        SHA1((unsigned char *)input, strlen(input), sha1_hash1);
        return sha1_hash1;
    }

    static char* base64_encode(const char *input) 
    {
        BIO *b64 = BIO_new(BIO_f_base64());
        BIO *bio = BIO_new(BIO_s_mem());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

        BIO_push(b64, bio);
        BIO_write(b64, input, strlen(input));
        BIO_flush(b64);

        BUF_MEM *bptr;
        BIO_get_mem_ptr(b64, &bptr);

        char *output = (char *)malloc(bptr->length + 1);
        memcpy(output, bptr->data, bptr->length);
        output[bptr->length] = '\0';

        BIO_free_all(b64);
        return output;
    }
};

class WebSocket 
{
public:
    static int parseFrame(char *buffer, int len, char *output) 
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

    static void encodeAndSend(const char *message, int len, int new_socket) 
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

        // Send the frame to the client
        send(new_socket, frame, len + 6, 0);
    }
};

class GameLogic 
{
public:
	static void initializeBoard(char (*board)[SIZE]) 
	{
		for (int i = 0; i < SIZE; i++) 
		{
		    for (int j = 0; j < SIZE; j++) 
		    {
		        board[i][j] = ' ';
		    }
		}
	}
	
	static int isGameOver(Player& player, char board[][SIZE]) 
	{
	    // Check rows
	    for (int i = 0; i < SIZE; i++) 
	    {
		if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
		    return 1; // Game over, player wins
	    }

	    // Check columns
	    for (int i = 0; i < SIZE; i++) 
	    {
		if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
		    return 1; // Game over, player wins
	    }

	    // Check diagonals
	    if ((board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ') ||
		(board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' '))
		return 1; // Game over, player wins

	    // Check if the board is full (draw)
	    for (int i = 0; i < SIZE; i++) 
	    {
		for (int j = 0; j < SIZE; j++) 
		{
		    if (board[i][j] == ' ')
		        return 0; // Game is not over yet
		}
	    }

	    return -1; // Game over, it's a draw
	}
	
	static void handleMove(Player& player1, Player& player2, char board[][SIZE], int currentPlayer) 
	{
	    while (true) 
	    {
		int new_socket = (currentPlayer == 1) ? player1.fd : player2.fd;
		char buffer[BUFFER_SIZE];
		char message[BUFFER_SIZE];

		// Receive move from the player
		int len = recv(new_socket, buffer, BUFFER_SIZE, 0);
		if (len <= 0) 
		{
		    perror("Receive failed");
		    exit(EXIT_FAILURE);
		}

		// Parse WebSocket frame and extract payload
		int payload_len = WebSocket::parseFrame(buffer, len, message);
		if (payload_len < 0) 
		{
		    perror("Invalid WebSocket frame");
		    exit(EXIT_FAILURE);
		}

		// Extract coordinates from the message
		int row = message[0] - '0';
		int col = message[2] - '0';

		// Update the game board with the move
		board[row][col] = (currentPlayer == 1) ? 'X' : 'O';
		
		char message4[BUFFER_SIZE];
		sprintf(message4, "%d,%d,%c", row, col, board[row][col]);
		printf("Mess: %s\n", message4);

		// Send the move to both players
		WebSocket::encodeAndSend(message4, strlen(message4), player1.fd);
		WebSocket::encodeAndSend(message4, strlen(message4), player2.fd);

		// Check for game-over conditions
		int result = isGameOver(player1, board);
		if (result == 1) 
		{
		    // Player 1 wins
		    printf("Player %d wins!\n", currentPlayer);
		    char message2[1024];
		    sprintf(message2, "Player %d wins!", currentPlayer);
		    char game_over_message[] = "Game Over";
		    WebSocket::encodeAndSend(message2, strlen(message2), player1.fd);
		    WebSocket::encodeAndSend(message2, strlen(message2), player2.fd);
		    WebSocket::encodeAndSend(game_over_message, strlen(game_over_message), player1.fd);
		    WebSocket::encodeAndSend(game_over_message, strlen(game_over_message), player2.fd);
		    break;
		} 
		else if (result == -1) 
		{
		    // It's a draw
		    char draw_message[] = "It's a draw!";
		    char game_over_message[] = "Game Over";
		    WebSocket::encodeAndSend(draw_message, strlen(draw_message), player1.fd);
		    WebSocket::encodeAndSend(draw_message, strlen(draw_message), player2.fd);
		    WebSocket::encodeAndSend(game_over_message, strlen(game_over_message), player1.fd);
		    WebSocket::encodeAndSend(game_over_message, strlen(game_over_message), player2.fd);
		    break;
		} 
		else 
		{
		    // Continue the game
		    char next_move_message[] = "Next move";
		    WebSocket::encodeAndSend(next_move_message, strlen(next_move_message), player1.fd);
		    WebSocket::encodeAndSend(next_move_message, strlen(next_move_message), player2.fd);
		    currentPlayer = (currentPlayer == 1) ? 2 : 1; // Switch player
		}
	    }
	}
	
	
};
	
	


class GameServer 
{
private:
    int num_players = 0;
    pthread_mutex_t mutex;
    char board[SIZE][SIZE];
    Player players[MAX_CLIENTS];
    std :: mutex board_mutex;
    
public:
    GameServer() 
    {
        pthread_mutex_init(&mutex, NULL);
    }
    
    void startGame() 
    {
        board_mutex.lock();
        Player& player1 = players[num_players - 2];
        Player& player2 = players[num_players - 1];
        char temp_board[SIZE][SIZE];
        memcpy(temp_board, board, sizeof(board));
        board_mutex.unlock();

        GameLogic::handleMove(player1, player2, temp_board, 1);
        GameLogic::handleMove(player2, player1, temp_board, 2);

        // Game over, disconnect clients and clean up
        close(player1.fd);
        close(player2.fd);

        // Reset the game board
        board_mutex.lock();
        GameLogic::initializeBoard(board);
        board_mutex.unlock();

        // Update player status
        player1.matched = 0;
        player2.matched = 0;
        player1.req = 0;
        player2.req = 0;
    }
    
    void handleClient(int new_socket) 
    {
    	    char buffer[BUFFER_SIZE];

	    // Receive handshake request from the client
	    if (recv(new_socket, buffer, BUFFER_SIZE, 0) <= 0) {
		perror("Receive failed");
		exit(EXIT_FAILURE);
	    }

	    // Handle WebSocket handshake
	    // Find WebSocket key from the request
	    char *key = strstr(buffer, "Sec-WebSocket-Key:");
	    if (key == NULL) {
		perror("WebSocket key not found");
		exit(EXIT_FAILURE);
	    }
	    key += strlen("Sec-WebSocket-Key:");
	    while (*key == ' ' || *key == ':')
		key++; // Skip spaces and colon
	    char *end = strchr(key, '\r');
	    if (end == NULL) {
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
	    unsigned char *sha1_result = CryptoUtils::sha1_hash(combined_key);

	    // Encode SHA-1 hash in Base64
	    char *base64_result = CryptoUtils::base64_encode((char *)sha1_result);

	    // Prepare handshake response
	    char handshake_response[BUFFER_SIZE];
	    sprintf(handshake_response, "HTTP/1.1 101 Switching Protocols\r\n"
		                         "Upgrade: websocket\r\n"
		                         "Connection: Upgrade\r\n"
		                         "Sec-WebSocket-Accept: %s\r\n"
		                         "\r\n", base64_result);
           
           printf("%s", handshake_response);

	    // Send handshake response to the client
	    send(new_socket, handshake_response, strlen(handshake_response), 0);
	    
	    handleWebSocket(new_socket);	
    }
    
    void handleWebSocket(int new_socket)
    {
    	    while (1) 
	    {
		char buffer[BUFFER_SIZE];

		// Receive message from the client
		int len = recv(new_socket, buffer, BUFFER_SIZE, 0);
		if (len <= 0) {
		    perror("Receive failed");
		    exit(EXIT_FAILURE);
		}

		// Parse WebSocket frame and extract payload
		int payload_len = WebSocket::parseFrame(buffer, len, buffer);
		if (payload_len < 0) 
		{
		    perror("Invalid WebSocket frame");
		    exit(EXIT_FAILURE);
		}

		// Handle PlayRequest message
		if (strcmp(buffer, "PlayRequest") == 0) 
		{
		    pthread_mutex_lock(&mutex);
		    Player new_player;
		    new_player.fd = new_socket;
		    new_player.matched = 0;
		    new_player.req = 1;
		    players[num_players++] = new_player;
		    pthread_mutex_unlock(&mutex);

		    if (num_players % 2 == 0) 
		    {
		        // Allocate board for the new game
		        board_mutex.lock();
		        GameLogic::initializeBoard(board);
		        board_mutex.unlock();

		        // Pair the last two clients and start the game
		        /*thread gameThread(&GameServer::startGame, this);
		        gameThread.detach();*/
		        
		        startGame();
		    }
		}
	    }
	    close(new_socket);
    }
              	

    void startServer() 
    {
	    int server_fd, cli_socket;
	    struct sockaddr_in address;
	    int addrlen = sizeof(address);

	    // Create socket
	    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	    {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	    }

	    // Set up server address and port
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

	    // Accept incoming connections and handle them
	    while (1) 
	    {
		cli_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
		if (cli_socket < 0) 
		{
		    perror("Accept failed");
		    exit(EXIT_FAILURE);
		}

		// Handle the client without creating a new thread
		std::thread client_thread(&GameServer::handleClient, this, cli_socket);
               client_thread.detach();
    	    }
    }
};

int main() 
{
    GameServer server;
    server.startServer();
    return 0;
}
