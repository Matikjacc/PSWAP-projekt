#ifndef GAME_U_H
#define GAME_U_H
#define BOARD_SIZE 3
#define MAX_PLAYERS_PER_LOBBY 2

typedef enum {
    LOBBY_WAITING,
    LOBBY_FULL,
    LOBBY_PLAYING,
    LOBBY_START
} LobbyStatus;

typedef enum {
    IN_PROGRESS,
    DRAW,
    WIN_X,
    WIN_O
} GameStatus;

typedef enum {
    CELL_X = 0,
    CELL_O,
    CELL_EMPTY
} Cell;

typedef struct {
    int player_fd;
    int player_id;
} Player;


typedef struct {
    int game_id;
    Cell board[BOARD_SIZE][BOARD_SIZE];
    Cell current_turn;
    GameStatus status;
} Game;


typedef struct {
    Player players[MAX_PLAYERS_PER_LOBBY];  // sockety graczy
    int player_count;
    int lobby_id;
    LobbyStatus status;
    Game game;
} Lobby;

typedef struct {
    int lobby_id;
    LobbyStatus status;
    Game game;
} GameInfo;

typedef struct {
    int player_id;
    char opponent_name[32];
} StartMessage;

typedef struct {
    int row;
    int col;
} __attribute__((packed)) Move;

#endif