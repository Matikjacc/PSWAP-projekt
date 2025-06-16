#ifndef GAME_CLIENT_H
#define GAME_CLIENT_H

#include "../common/game.h"

void game_client_init(Game *game);

int game_client_update(Game *game, const char *data);

void game_client_draw(const Game *game);

Cell char_to_cell(char c);

#endif // GAME_CLIENT_H