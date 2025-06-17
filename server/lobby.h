#ifndef LOBBY_H
#define LOBBY_H

#include "../common/game.h"

#define MAX_LOBBIES 10
#define MAX_PLAYERS_PER_LOBBY 2

typedef struct {
    Player players[MAX_PLAYERS_PER_LOBBY];  // sockety graczy
    int player_count;
    int lobby_id;
    LobbyStatus status;
    Game game;
} Lobby;

void lobby_init_all();
int lobby_join(int player_fd);                // zwraca indeks lobby lub -1
void lobby_remove_player(int player_fd);      // usuwa gracza ze wszystkich lobby
Lobby* lobby_get(int index);                  // dostęp do konkretnego lobby

#endif // LOBBY_H