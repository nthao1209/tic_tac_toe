# tic_tac_toe
***Tic-Tac-Toe (10x10) Server-Client Game***
***Project Description***

This project implements a two-player Tic-Tac-Toe game on a 10x10 grid, where players aim to align five marks ('X' or 'O') in a row, column, or diagonal to win. The game uses a client-server architecture:

***Server (server.c)***: Manages game state, validates moves, checks for winners, and communicates with clients via TCP sockets.
***Client (client.c)***: Provides a graphical interface using SDL2, allowing players to make moves by clicking on the grid and displaying the game state.

The server supports two players, assigns them as Player 1 ('X') and Player 2 ('O'), and handles turn-based gameplay. The client renders a 10x10 grid, captures mouse clicks for move input, and displays status messages (e.g., whose turn, game result).
***Features***

10x10 game board with a 5-in-a-row win condition.
Client-server communication over TCP (port 8888).
Graphical client interface with SDL2 for mouse-based input.
Real-time board updates and turn notifications.
Support for win, loss, or draw outcomes.

***Prerequisites***

***Operating System***: Linux (e.g., Ubuntu) is recommended. The code is portable to macOS or Windows with SDL2 setup.
***Compiler***: GCC or compatible C compiler.


***Libraries***:

SDL2 (libsdl2-dev)

SDL2_ttf (libsdl2-ttf-dev)

Standard C libraries (including libm for math functions)


***Installation***

***Install Dependencies (Ubuntu/Debian)***:
sudo apt-get update
sudo apt-get install gcc libsdl2-dev libsdl2-ttf-dev fonts-dejavu


***For macOS*** (using Homebrew):brew install sdl2 sdl2_ttf


***For Windows:*** Install SDL2 and SDL2_ttf via MSYS2 or download development libraries and configure with your IDE/compiler.

***Compilation***

Compile the Server:
    gcc server.c -o server

This creates the server executable. No additional libraries are required for the server.

Compile the Client:
    gcc client.c -o client -lSDL2 -lSDL2_ttf -lm


-lSDL2: Links SDL2 for graphics and input.
-lSDL2_ttf: Links SDL2_ttf for text rendering.
-lm: Links the math library for cos and sin (used to draw 'O').
Creates the client executable.

Alternatively, use pkg-config for SDL2 flags:
    gcc client.c -o client $(pkg-config --libs --cflags sdl2 SDL2_ttf) -lm



***Running the Game***

Start the Server:
  ./server

The server listens on 127.0.0.1:8888 and waits for two clients to connect. It prints messages like:
Server listening on port 8888...
Client 1 connected
Client 2 connected


Run Two Client Instances:Open two separate terminals and run:
./client


Each client connects to the server and is assigned a player ID (1 or 2).
The client window (600x650 pixels) shows a 10x10 grid and a status message (e.g., "Your turn (Player 1)").
If running on different machines, update HOST in client.c to the server's IP address.


***Playing the Game:***

Player 1 ('X', red) and Player 2 ('O', blue) take turns.
Click an empty cell on the grid to make a move (only during your turn).
The server validates moves and updates both clients.
The game ends when:
A player gets 5 marks in a row, column, or diagonal (win).
The board is full (100 moves, draw).


The result is displayed (e.g., "Game Over: Player 1 wins"), and clients exit after 2 seconds.


***Stopping the Game:***

Close client windows or press Ctrl+C in the server terminal.
The server and clients clean up sockets and SDL resources on exit.



***Project Structure***

    server.c: Server implementation (game logic, socket communication).
    client.c: Client implementation (SDL2-based GUI, mouse input, socket communication).
    README.md: This file.

***Notes***

***Font Issues:*** If the client fails to load the font, ensure the font path in client.c is correct or install the DejaVu fonts (sudo apt-get install fonts-dejavu).
***Network Setup:*** For remote play, ensure the server is accessible (update HOST in client.c and check firewall settings).
***Error Handling:*** Invalid moves are ignored by the server. Clients exit on connection errors or window closure.
***Customization:*** Modify BOARD_SIZE, WIN_LENGTH, or CELL_SIZE in both files consistently to change the game (requires recompilation).
***Limitations:*** The server supports only two players. Clients are native (not browser-based).

***Troubleshooting***

Compilation Error: "undefined reference to cos":Ensure -lm is included:gcc client.c -o client -lSDL2 -lSDL2_ttf -lm


SDL2:Verify SDL2 and SDL2_ttf installation:pkg-config --libs --cflags sdl2 SDL2_ttf

Connection Issues:Ensure the server is running before starting clients and that HOST/PORT match.


