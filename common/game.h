#define BOARD_SIZE 3

typedef enum {
    PLAYER_O = 0,
    PLAYER_X
} PlayerChar;

typedef enum {
    CELL_EMPTY = 0,
    CELL_X,
    CELL_O
} Cell;

typedef enum {
    IN_PROGRESS,
    DRAW,
    WIN_X,
    WIN_O
} GameStatus;

typedef struct {
    int id;
    char other_player_name[32];
    PlayerChar player_char;
} GameInit;

typedef struct {
    int game_id;
    Cell board[BOARD_SIZE][BOARD_SIZE];
    Cell current_turn;
    GameStatus status;
} Game;

typedef struct {
    int player_fd;
    int player_id;
    PlayerChar player_char;
} Player;

typedef enum {
    LOBBY_WAITING,
    LOBBY_FULL,
    LOBBY_PLAYING,
    LOBBY_START
} LobbyStatus;

typedef struct {
    int lobby_id;
    LobbyStatus status;
    Game game;
} GameInfo;

typedef struct {
    int player_id;
    int player_turn;
} StartMessage;