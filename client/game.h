#ifndef CLIENT_GAME_H
#define CLIENT_GAME_H

#include "../common/game.h"
#include "client.h"


void game_client_init(int sockfd);
int game_client_update(Game *game, const char *data);
void game_client_draw(const Game *game);
Cell char_to_cell(char c);
const char* status_to_string(GameStatus status);
GameStatus string_to_status(const char *str);
void start_game(GameInfo *game_info, int sockfd);

#endif // CLIENT_GAME_H