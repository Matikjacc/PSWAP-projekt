#ifndef GAME_H
#define GAME_H

#define BOARD_SIZE 3

typedef enum {
    EMPTY = 0,
    PLAYER_X,
    PLAYER_O
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

void game_init(Game *game);

int game_make_move(Game *game, int row, int col);

GameStatus game_check_status(Game *game);

Cell game_current_turn(const Game *game);

Cell game_get_cell(const Game *game, int row, int col);

#endif // GAME_H