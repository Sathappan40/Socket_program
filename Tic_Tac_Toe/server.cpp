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

#define PORT 8080
#define BUFFER_SIZE 1024
#define SIZE 3
#define MAX_CLIENTS 10

using namespace std;

class Player {
public:
    int fd;
    int matched;
    char board[SIZE][SIZE];
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for synchronization
int num_client = 0;
//int num_players = 0;
int client_sockets[MAX_CLIENTS];



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
	    //printf("Frame: %s\n",frame);

	    // Send the frame to the client
	    send(new_socket, frame, len + 6, 0);
     }
};

class GameLogic 
{
public:
    static void initializeBoard(Player& player) 
    {
            pthread_mutex_lock(&mutex); // Lock mutex
	     for (int i = 0; i < SIZE; i++) 
	     {
        	for (int j = 0; j < SIZE; j++) 
        	{
            		player.board[i][j] = ' ';
        	}
    	     }

	   
	    pthread_mutex_unlock(&mutex); // Unlock mutex
    }

    static int isGameOver(Player& player) 
    {
    	// Check rows
        for (int i = 0; i < SIZE; i++) {
            if (player.board[i][0] == player.board[i][1] && player.board[i][1] == player.board[i][2] && player.board[i][0] != ' ')
                return 1; // Game over, player wins
        }

        // Check columns
        for (int i = 0; i < SIZE; i++) {
            if (player.board[0][i] == player.board[1][i] && player.board[1][i] == player.board[2][i] && player.board[0][i] != ' ')
                return 1; // Game over, player wins
        }

        // Check diagonals
        if ((player.board[0][0] == player.board[1][1] && player.board[1][1] == player.board[2][2] && player.board[0][0] != ' ') ||
            (player.board[0][2] == player.board[1][1] && player.board[1][1] == player.board[2][0] && player.board[0][2] != ' '))
            return 1; // Game over, player wins

        // Check if the board is full (draw)
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                if (player.board[i][j] == ' ')
                    return 0; // Game is not over yet
            }
        }

        return -1; // Game over, it's a draw
    }
        
    

    void handleMove(Player& player1, Player& player2, std::vector<Player>& players, int num_players) 
    {
    	int currentPlayer = 1;

        while (1) 
        {
		int new_socket = (currentPlayer == 1) ? player1.fd : player2.fd;
		char buffer[BUFFER_SIZE];
		char message[BUFFER_SIZE];
		char message4[BUFFER_SIZE];
	

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
		player1.board[row][col] = (currentPlayer == 1) ? 'X' : 'O';
		
		sprintf(message4, "%d,%d,%c", row, col, player1.board[row][col]);
		printf ("Mess: %s\n", message4);

		// Send the move to both players
		for (int i = 0; i < num_players; i++) 
		{
		    if (&player1 == &players[i] || &player2 == &players[i]) 
		    {
		    	WebSocket::encodeAndSend(message4, strlen(message4), players[i].fd);
		        
		    }
		}
		
		char message2[1096];

		// Check for game-over conditions
		int result = GameLogic::isGameOver(player1);
		if (result == 1) 
		{
		    // Player 1 wins
		     printf("Player %d wins!\n", currentPlayer);
	             sprintf(message2, "Player %d wins!", currentPlayer);
		    char game_over_message[] = "Game Over";
		    for (int i = 0; i < num_players; i++) {
		        if (&player1 == &players[i] || &player2 == &players[i]) {
		            WebSocket::encodeAndSend(message2, strlen(message2), players[i].fd);
		            WebSocket::encodeAndSend(game_over_message, strlen(game_over_message), players[i].fd);
		        }
		    }
		    break;
		} 
		else if (result == -1) 
		{
		    // It's a draw
		    char draw_message[] = "It's a draw!";
		    char game_over_message[] = "Game Over";
		    for (int i = 0; i < num_players; i++) 
		    {
		        if (&player1 == &players[i] || &player2 == &players[i]) 
		        {
		            WebSocket::encodeAndSend(draw_message, strlen(draw_message), players[i].fd);
		            WebSocket::encodeAndSend(game_over_message, strlen(game_over_message), players[i].fd);
		        }
		    }
		    break;
		} 
		else 
		{
		    // Continue the game
		    char next_move_message[] = "Next move";
		    for (int i = 0; i < num_players; i++) 
		    {
		        if (&player1 == &players[i] || &player2 == &players[i]) 
		        {
		            WebSocket::encodeAndSend(next_move_message, strlen(next_move_message), players[i].fd);
		        }
		    }
		    currentPlayer = (currentPlayer == 1) ? 2 : 1; // Switch player
		}
         }
    }
};

class GameServer 
{
private:
    int num_players = 0;
    int client_sockets[MAX_CLIENTS];
    int num_client = 0;
    pthread_mutex_t mutex;
    GameLogic game;
public:
    GameServer()
    {
        pthread_mutex_init(&mutex, NULL);
    }
	
    std::vector<Player> players;
    /*int getNum ()
    {
	return num_players;
    }*/
    void handle_playreq(Player& player) 
    {
           int flag = 0,i;
           cout<<player.fd<<endl;
           cout<<GameServer::players[1].fd<<endl;

	    if (!player.matched) 
	    {
		if (num_players >= 2) 
		{
		    for ( i = 0; i < num_players; i++) 
		    {
		        if (!GameServer::players[i].matched && player.fd != GameServer::players[i].fd) 
		        {
		            // Match found
		            player.matched = 1;
		            GameServer::players[i].matched = 1;
		            flag = 1;
		            
		            GameLogic :: initializeBoard(player);
		            GameLogic :: initializeBoard(GameServer::players[i]);
		            
		            break;
		        }
		    }
		}
	    }

	    if (flag) 
	    {
		game.handleMove(player, players[i], players, num_players);
		// Reset match status after the game
		player.matched = 0;
		GameServer::players[i].matched = 0;
	    }
    }
    
    /*static void* threadEntryPoint(void* arg) 
    {
    	 int new_sock = *(int*)arg;
    	 GameServer server;
    	 
    	 cout << "(((" << server.getNum () << endl;
         server.handleClient(new_sock);
         return nullptr;
    }*/

    void handleClient(int new_socket) 
    {
	    
	    if (num_client < MAX_CLIENTS) 
    	    {
            	client_sockets[num_client++] = new_socket;
            } 
	    
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
	    char *base64_result = CryptoUtils::base64_encode((char*)sha1_result);
	    
	    // Prepare handshake response
	    char handshake_response[BUFFER_SIZE];
	    sprintf(handshake_response, "HTTP/1.1 101 Switching Protocols\r\n"
		                         "Upgrade: websocket\r\n"
		                         "Connection: Upgrade\r\n"
		                         "Sec-WebSocket-Accept: %s\r\n"
		                         "\r\n", base64_result);
	    
	    // Send handshake response to the client
	    send(new_socket, handshake_response, strlen(handshake_response), 0);
	    
	    // Once handshake is done, handle WebSocket communication
	    GameServer::handle_websocket(new_socket);
    }

    void handle_websocket(int new_socket) 
    {
    	cout << "@@@" << num_players << endl;
	    char buffer[BUFFER_SIZE];
	    int id = num_players;
	    cout<<GameServer::players.size()<<endl;

	    // Add the new player to the players vector
	    Player new_player;
	    new_player.fd = new_socket;
	    new_player.matched = 0;
	    GameServer::players.push_back(new_player);
	    cout<<GameServer::players.size()<< " "<< &new_player<<endl;
	    
	    cout<<"hello"<<new_player.fd<<endl;
	    num_players++;
		cout << "$$$" << num_players << endl;
	    while (1) 
	    {
		char message[BUFFER_SIZE];

		// Receive message from the client
		int len = recv(new_socket, buffer, BUFFER_SIZE, 0);
		if (len <= 0) {
		    perror("Receive failed");
		    exit(EXIT_FAILURE);
		}

		// Parse WebSocket frame and extract payload
		int payload_len = WebSocket::parseFrame(buffer, len, message);
		if (payload_len < 0) {
		    perror("Invalid WebSocket frame");
		    exit(EXIT_FAILURE);
		}

		// Handle PlayRequest message
		if (strcmp(message, "PlayRequest") == 0) {
		    while (1) {
		        if (GameServer::players[id].matched != 0) {
		            break;
		        }
		       
		        GameServer::handle_playreq(GameServer::players[id]);
		    }
		}

		// Check if any player is matched and wait until the game ends
		while (GameServer::players[id].matched != 0);

		// Continue WebSocket communication with the client
		// Add more message handling logic as needed
           }
           close(GameServer::players[id].fd);
           pthread_exit(NULL);
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

		// Handle each client in a separate thread
		//pthread_t tid;
		//int* new_socket = new int(cli_socket);
		/*if (pthread_create(&tid, NULL, &threadEntryPoint, (void*)new_socket) != 0) {
    			perror("pthread_create failed");
    			close(cli_socket);
    			delete new_socket;
		}*/
		
		thread myThread(&GameServer::handleClient,this,cli_socket);
                myThread.detach();
        
		
		/*if(pthread_detach(tid) != 0)
		    perror("pthread_detach");*/
	    }
     }
};


int main() {
    GameServer server;
    //cout << server.getNum () << endl;
    server.startServer();
    return 0;
}

