#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "lobby.h"
#include "../common/protocol.h"
#include <stdio.h>

static Lobby lobbies[MAX_LOBBIES];

void lobby_init_all() {
    for (int i = 0; i < MAX_LOBBIES; i++) {
        lobbies[i].id = -1;
        // Fix: Use proper pointer arithmetic for player_fd array
        memset(&lobbies[i].players->player_fd, -1, sizeof(lobbies[i].players->player_fd));
        lobbies[i].player_count = 0;
        lobbies[i].status = LOBBY_WAITING;
        //game_server_init(&lobbies[i].game);  // zakładamy, że masz game_server_init
    }
}

int lobby_join(int player_fd) {
    TLVMessage msg;
    for (int i = 0; i < MAX_LOBBIES; ++i) {
        Lobby *lobby = &lobbies[i];

        if (lobby->status == LOBBY_WAITING && lobby->player_count < MAX_PLAYERS_PER_LOBBY) {
            lobby->players[lobby->player_count++].player_fd = player_fd;

            if (lobby->player_count == MAX_PLAYERS_PER_LOBBY) {
                lobby->status = LOBBY_FULL;
                lobby->id = rand();
                msg.type = MSG_JOIN_LOBBY_SUCCESS;
                msg.length = sizeof(lobby->id);
                lobby->status = LOBBY_PLAYING;
                
            }

            return i;  // indeks lobby, do którego dołączono
        }
    }
    fprintf(stderr, "Wszystkie lobby zostały zapełnione.\n");
    msg.type = MSG_ALL_LOBBIES_FULL;
    msg.length = 0;
    // send() is now properly declared via sys/socket.h
    send(player_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
    return -1;
}

void lobby_remove_player(int player_fd) {
    for (int i = 0; i < MAX_LOBBIES; ++i) {
        Lobby *lobby = &lobbies[i];
        for (int j = 0; j < MAX_PLAYERS_PER_LOBBY; ++j) {
            if (lobby->players[j].player_fd == player_fd) {
                // usuń gracza
                lobby->players[j].player_fd = -1;
                // przesuwanie graczy w tablicy
                for (int k = j; k < MAX_PLAYERS_PER_LOBBY - 1; ++k)
                    lobby->players[k].player_fd = lobby->players[k+1].player_fd;
                lobby->players[MAX_PLAYERS_PER_LOBBY - 1].player_fd = -1;
                lobby->player_count--;

                if (lobby->player_count == 0) {
                    lobby->status = LOBBY_WAITING;
                } else if (lobby->status == LOBBY_PLAYING) {
                    lobby->status = LOBBY_WAITING; 
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

