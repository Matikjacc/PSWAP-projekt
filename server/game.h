#ifndef GAME_H
#define GAME_H

#define BOARD_SIZE 3
#define MAX_LOBBIES 10
#include "../common/game.h"

void game_init(Game *game);

int game_make_move(Game *game, int row, int col);

GameStatus game_check_status(Game *game);

Cell game_current_turn(const Game *game);

Cell game_get_cell(const Game *game, int row, int col);

#endif // GAME_H