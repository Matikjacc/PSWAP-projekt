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
    int id;
    Cell board[BOARD_SIZE][BOARD_SIZE];
    Cell current_turn;
    GameStatus status;
} Game;

typedef struct {
    int player_fd;
    PlayerChar player_char;
} Player;