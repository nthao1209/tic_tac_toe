#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define M_PI 3.14159265358979323846
#define HOST "127.0.0.1"
#define PORT 8888
#define BUFFER_SIZE 128
#define BOARD_SIZE 10
#define CELL_SIZE 60
#define WINDOW_WIDTH (BOARD_SIZE * CELL_SIZE)
#define WINDOW_HEIGHT (BOARD_SIZE * CELL_SIZE + 50)

// Message types
#define MOVE 0x02
#define STATE_UPDATE 0x03
#define RESULT 0x04
#define TURN_NOTIFICATION 0x05

// Game state
int board[BOARD_SIZE][BOARD_SIZE];
int player_id = 0;
int is_my_turn = 0;
int game_over = 0;
char status_message[64] = "Waiting to connect...";

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;
int sockfd;

// Initialize SDL and connect to server
int init() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 0;
    }
    if (TTF_Init() < 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
        return 0;
    }
    
    window = SDL_CreateWindow("Tic-Tac-Toe Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 0;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return 0;
    }
    
    font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24);
    if (!font) {
        printf("Font loading failed: %s\n", TTF_GetError());
        return 0;
    }
    
    // Initialize socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 0;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST, &server_addr.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 0;
    }
    
    // Receive player ID
    unsigned char buffer[BUFFER_SIZE];
    int bytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes > 0) {
        player_id = buffer[0];
        sprintf(status_message, "Player %d: Waiting for opponent", player_id);
    }
    
    return 1;
}

// Draw the game board
void draw_board() {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    
    // Draw grid
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    for (int i = 0; i <= BOARD_SIZE; i++) {
        SDL_RenderDrawLine(renderer, 0, i * CELL_SIZE, WINDOW_WIDTH, i * CELL_SIZE);
        SDL_RenderDrawLine(renderer, i * CELL_SIZE, 0, i * CELL_SIZE, WINDOW_HEIGHT - 50);
    }
    
    // Draw pieces
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == 1) {
                // Draw X
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderDrawLine(renderer, j * CELL_SIZE + 10, i * CELL_SIZE + 10,
                                  j * CELL_SIZE + CELL_SIZE - 10, i * CELL_SIZE + CELL_SIZE - 10);
                SDL_RenderDrawLine(renderer, j * CELL_SIZE + 10, i * CELL_SIZE + CELL_SIZE - 10,
                                  j * CELL_SIZE + CELL_SIZE - 10, i * CELL_SIZE + 10);
            } else if (board[i][j] == 2) {
                // Draw O
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                SDL_Rect rect = {j * CELL_SIZE + 10, i * CELL_SIZE + 10, CELL_SIZE - 20, CELL_SIZE - 20};
                for (int t = 0; t < 360; t += 5) {
                    float rad = t * M_PI / 180;
                    int x1 = rect.x + rect.w / 2 + (rect.w / 2) * cos(rad);
                    int y1 = rect.y + rect.h / 2 + (rect.h / 2) * sin(rad);
                    rad = (t + 5) * M_PI / 180;
                    int x2 = rect.x + rect.w / 2 + (rect.w / 2) * cos(rad);
                    int y2 = rect.y + rect.h / 2 + (rect.h / 2) * sin(rad);
                    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
                }
            }
        }
    }
    
    // Draw status message
    SDL_Color text_color = {0, 0, 0, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, status_message, text_color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect text_rect = {10, WINDOW_HEIGHT - 40, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &text_rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    
    SDL_RenderPresent(renderer);
}

// Handle server messages
void handle_server_messages() {
    unsigned char buffer[BUFFER_SIZE];
    fd_set read_fds;
    struct timeval tv = {0, 0};
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);
    
    if (select(sockfd + 1, &read_fds, NULL, NULL, &tv) > 0) {
        int bytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) {
            sprintf(status_message, "Disconnected from server");
            game_over = 1;
            return;
        }
        
        int msg_type = buffer[0];
        if (msg_type == STATE_UPDATE) {
            int idx = 1;
            for (int i = 0; i < BOARD_SIZE; i++)
                for (int j = 0; j < BOARD_SIZE; j++)
                    board[i][j] = buffer[idx++];
            sprintf(status_message, is_my_turn ? "Your turn (Player %d)" : "Opponent's turn", player_id);
        } else if (msg_type == TURN_NOTIFICATION) {
            is_my_turn = 1;
            sprintf(status_message, "Your turn (Player %d)", player_id);
        } else if (msg_type == RESULT) {
            int winner = buffer[1];
            if (winner == 0x03)
                sprintf(status_message, "Game Over: Draw");
            else
                sprintf(status_message, "Game Over: Player %d wins", winner);
            game_over = 1;
        }
    }
}

// Send move to server
void send_move(int row, int col) {
    unsigned char buffer[BUFFER_SIZE] = {0};
    buffer[0] = MOVE;
    buffer[1] = row;
    buffer[2] = col;
    send(sockfd, buffer, BUFFER_SIZE, 0);
    is_my_turn = 0;
    sprintf(status_message, "Waiting for opponent's move");
}

int main() {
    if (!init()) {
        printf("Initialization failed\n");
        return 1;
    }
    
    // Initialize board
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            board[i][j] = 0;
    
    SDL_Event event;
    int running = 1;
    
    while (running) {
        // Handle server messages
        handle_server_messages();
        
        // Handle SDL events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEBUTTONDOWN && is_my_turn && !game_over) {
                int x = event.button.x;
                int y = event.button.y;
                if (y < WINDOW_HEIGHT - 50) { // Click within board area
                    int row = y / CELL_SIZE;
                    int col = x / CELL_SIZE;
                    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && board[row][col] == 0) {
                        send_move(row, col);
                    }
                }
            }
        }
        
        // Draw board
        draw_board();
        
        if (game_over) {
            SDL_Delay(2000); // Show result for 2 seconds
            running = 0;
        }
        
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    close(sockfd);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}