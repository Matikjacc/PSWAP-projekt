#include <string.h>
#include "lobby.h"

static Lobby lobbies[MAX_LOBBIES];

void lobby_init_all() {
    for (int i = 0; i < MAX_LOBBIES; ++i) {
        lobbies[i].player_count = 0;
        lobbies[i].status = LOBBY_WAITING;
        memset(lobbies[i].player_fds, -1, sizeof(lobbies[i].player_fds));
        game_server_init(&lobbies[i].game);  // zakładamy, że masz game_server_init
    }
}

int lobby_join(int player_fd) {
    for (int i = 0; i < MAX_LOBBIES; ++i) {
        Lobby *lobby = &lobbies[i];

        if (lobby->status == LOBBY_WAITING && lobby->player_count < MAX_PLAYERS_PER_LOBBY) {
            lobby->player_fds[lobby->player_count++] = player_fd;

            if (lobby->player_count == MAX_PLAYERS_PER_LOBBY) {
                lobby->status = LOBBY_PLAYING;
                game_server_init(&lobby->game);
                // można dodać start gry itp.
            }

            return i;  // indeks lobby, do którego dołączono
        }
    }
    return -1;  // brak dostępnych lobby
}

void lobby_remove_player(int player_fd) {
    for (int i = 0; i < MAX_LOBBIES; ++i) {
        Lobby *lobby = &lobbies[i];
        for (int j = 0; j < MAX_PLAYERS_PER_LOBBY; ++j) {
            if (lobby->player_fds[j] == player_fd) {
                // usuń gracza
                lobby->player_fds[j] = -1;
                // przesuwanie graczy w tablicy
                for (int k = j; k < MAX_PLAYERS_PER_LOBBY - 1; ++k)
                    lobby->player_fds[k] = lobby->player_fds[k + 1];
                lobby->player_fds[MAX_PLAYERS_PER_LOBBY - 1] = -1;
                lobby->player_count--;

                if (lobby->player_count == 0) {
                    lobby->status = LOBBY_WAITING;
                } else if (lobby->status == LOBBY_PLAYING) {
                    lobby->status = LOBBY_WAITING; // lub dodać flagę "przerwana gra"
                }

                return;
            }
        }
    }
}

Lobby* lobby_get(int index) {
    if (index < 0 || index >= MAX_LOBBIES) return NULL;
    return &lobbies[index];
}
