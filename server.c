#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8888
#define BUFFER_SIZE 128
#define BOARD_SIZE 10
#define WIN_LENGTH 5

// Message types
#define MOVE 0x02
#define STATE_UPDATE 0x03
#define RESULT 0x04
#define TURN_NOTIFICATION 0x05

// Game board
int board[BOARD_SIZE][BOARD_SIZE];
int move_count = 0;
int current_player = 1;

// Initialize the game board
void init_board() {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            board[i][j] = 0;
}

// Check for a winner
int check_winner() {
    // Check rows
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j <= BOARD_SIZE - WIN_LENGTH; j++) {
            if (board[i][j] != 0) {
                int match = 1;
                for (int k = 1; k < WIN_LENGTH; k++) {
                    if (board[i][j] != board[i][j + k]) {
                        match = 0;
                        break;
                    }
                }
                if (match) return board[i][j];
            }
        }
    }
    // Check columns
    for (int j = 0; j < BOARD_SIZE; j++) {
        for (int i = 0; i <= BOARD_SIZE - WIN_LENGTH; i++) {
            if (board[i][j] != 0) {
                int match = 1;
                for (int k = 1; k < WIN_LENGTH; k++) {
                    if (board[i][j] != board[i + k][j]) {
                        match = 0;
                        break;
                    }
                }
                if (match) return board[i][j];
            }
        }
    }
    // Check main diagonals
    for (int i = 0; i <= BOARD_SIZE - WIN_LENGTH; i++) {
        for (int j = 0; j <= BOARD_SIZE - WIN_LENGTH; j++) {
            if (board[i][j] != 0) {
                int match = 1;
                for (int k = 1; k < WIN_LENGTH; k++) {
                    if (board[i][j] != board[i + k][j + k]) {
                        match = 0;
                        break;
                    }
                }
                if (match) return board[i][j];
            }
        }
    }
    // Check anti-diagonals
    for (int i = 0; i <= BOARD_SIZE - WIN_LENGTH; i++) {
        for (int j = WIN_LENGTH - 1; j < BOARD_SIZE; j++) {
            if (board[i][j] != 0) {
                int match = 1;
                for (int k = 1; k < WIN_LENGTH; k++) {
                    if (board[i][j] != board[i + k][j - k]) {
                        match = 0;
                        break;
                    }
                }
                if (match) return board[i][j];
            }
        }
    }
    return 0; // No winner
}

// Send board state to both clients
void send_state_update(int client1_sock, int client2_sock) {
    unsigned char buffer[BUFFER_SIZE];
    buffer[0] = STATE_UPDATE;
    int idx = 1;
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            buffer[idx++] = board[i][j];
    send(client1_sock, buffer, BUFFER_SIZE, 0);
    send(client2_sock, buffer, BUFFER_SIZE, 0);
}

// Send result to both clients
void send_result(int client1_sock, int client2_sock, int winner) {
    unsigned char buffer[BUFFER_SIZE];
    buffer[0] = RESULT;
    buffer[1] = (winner > 0) ? winner : 0x03; // 0x03 for draw
    send(client1_sock, buffer, BUFFER_SIZE, 0);
    send(client2_sock, buffer, BUFFER_SIZE, 0);
}

// Send turn notification to the current player
void notify_turn(int client_sock) {
    unsigned char buffer[BUFFER_SIZE];
    buffer[0] = TURN_NOTIFICATION;
    send(client_sock, buffer, BUFFER_SIZE, 0);
}

// Validate and process a move
int process_move(unsigned char *buffer, int player) {
    int row = buffer[1];
    int col = buffer[2];
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE || board[row][col] != 0)
        return 0; // Invalid move
    board[row][col] = player;
    move_count++;
    return 1; // Valid move
}

int main() {
    int server_sock, client1_sock, client2_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    unsigned char buffer[BUFFER_SIZE];

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    // Listen for connections
    if (listen(server_sock, 2) < 0) {
        perror("Listen failed");
        exit(1);
    }
    printf("Server listening on port %d...\n", PORT);

    // Accept first client
    client1_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
    if (client1_sock < 0) {
        perror("Accept failed for client 1");
        exit(1);
    }
    printf("Client 1 connected\n");

    // Accept second client
    client2_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
    if (client2_sock < 0) {
        perror("Accept failed for client 2");
        exit(1);
    }
    printf("Client 2 connected\n");

    // Initialize game
    init_board();
    send_state_update(client1_sock, client2_sock);

    // Game loop
    while (1) {
        int current_sock = (current_player == 1) ? client1_sock : client2_sock;
        notify_turn(current_sock);

        // Receive move
        int bytes_received = recv(current_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Client %d disconnected\n", current_player);
            break;
        }

        if (buffer[0] != MOVE) {
            printf("Invalid message type from player %d\n", current_player);
            continue;
        }

        // Process move
        if (!process_move(buffer, current_player)) {
            printf("Invalid move by player %d\n", current_player);
            continue; // Ask for another move
        }

        // Update game state
        send_state_update(client1_sock, client2_sock);

        // Check for winner or draw
        int winner = check_winner();
        if (winner > 0 || move_count == BOARD_SIZE * BOARD_SIZE) {
            send_result(client1_sock, client2_sock, winner);
            break;
        }

        // Switch player
        current_player = (current_player == 1) ? 2 : 1;
    }

    // Clean up
    close(client1_sock);
    close(client2_sock);
    close(server_sock);
    printf("Game ended\n");
    return 0;
}