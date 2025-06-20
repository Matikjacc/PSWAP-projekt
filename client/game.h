#ifndef CLIENT_GAME_H
#define CLIENT_GAME_H

#include "../common/game.h"

void game_client_init(int sockfd);
void game_client_draw(const Game *game);
void start_game(GameInfo *game_info, int sockfd);

#endif