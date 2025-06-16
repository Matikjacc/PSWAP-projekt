#define BOARD_SIZE 3

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
    Cell board[BOARD_SIZE][BOARD_SIZE];
    Cell current_turn;
    GameStatus status;
} Game;