#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "lobby.h"
#include "storage.h"
#include "../common/protocol.h"
#include <stdio.h>

Lobby lobbies[MAX_LOBBIES];

void lobby_init_all() {
    for (int i = 0; i < MAX_LOBBIES; i++) {
        lobbies[i].lobby_id = -1;
        // Fix: Use proper pointer arithmetic for player_fd array
        memset(&lobbies[i].players->player_fd, -1, sizeof(lobbies[i].players->player_fd));
        lobbies[i].player_count = 0;
        lobbies[i].status = LOBBY_WAITING;
        lobbies[1].game.game_id = -1;
        memset(&lobbies[i].game.board, CELL_EMPTY, sizeof(lobbies[i].game.board));
        lobbies[i].game.current_turn = CELL_X;
    }
}

int lobby_join(int player_fd, int player_id) {
    TLVMessage msg;
    for (int i = 0; i < MAX_LOBBIES; ++i) {
        Lobby *lobby = &lobbies[i];

        if (lobby->status == LOBBY_WAITING && lobby->player_count < MAX_PLAYERS_PER_LOBBY) {
            lobby->players[lobby->player_count++].player_fd = player_fd;
            lobby->players[lobby->player_count - 1].player_id = player_id;

            GameInfo game_info;
            Game *game = &lobby->game;
            
            
            if (lobby->player_count == 1) {
                lobby->status = LOBBY_WAITING;
                lobby->lobby_id = rand();
                lobby->game.game_id = rand();
                game_info.game.game_id = lobby->game.game_id;
                printf("Tworzę nowe lobby o ID: %d\n", lobby->lobby_id);
                game->current_turn = CELL_X;
                game->status = IN_PROGRESS;
                lobby->game.status = IN_PROGRESS;
                game_init(game);
            }
            if (lobby->player_count == MAX_PLAYERS_PER_LOBBY) {
                lobby->status = LOBBY_START;
            }

            game_info.status = lobby->status;
            game_info.lobby_id = lobby->lobby_id;
            msg.length = sizeof(game_info);
            msg.type = MSG_JOIN_LOBBY_SUCCESS;
            game_info.game = *game;

            ssize_t header_size = sizeof(msg.type) + sizeof(msg.length);
            if (send(player_fd, &msg, header_size, 0) < 0) {
                perror("send join lobby header");
                return -1;
            }

            memcpy(msg.value, &game_info, sizeof(game_info));
            if(msg.length > 0) {
                if (send(player_fd, msg.value, msg.length, 0) < 0) {
                    perror("send join lobby value");
                    return -1;
                }
            }
            
            if (lobby->player_count == MAX_PLAYERS_PER_LOBBY) {
                printf("Lobby %d jest pełne. Wysyłam wiadomość do pierwszego gracza.\n", lobby->lobby_id);
                TLVMessage other_client_msg;
                other_client_msg.type = MSG_LOBBY_READY;
                other_client_msg.length = 0;
                other_client_msg.value[0] = '\0';
                if(send(lobby->players[0].player_fd, &other_client_msg, sizeof(other_client_msg), 0) < 0) {
                    perror("send lobby ready to first player");
                    return -1;
                }
                lobby->status = LOBBY_START;
                start_game(lobby);
            }
            printf("Wysłano wiadomość o dołączeniu do lobby.\n");
            return i; 
        }
    }
    fprintf(stderr, "Wszystkie lobby zostały zapełnione.\n");

    msg.type = MSG_ALL_LOBBIES_FULL;
    msg.length = 0;
    // send() is now properly declared via sys/socket.h
    send(player_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
    return -1;
}

void start_game(Lobby *lobby) {
    if (lobby->status != LOBBY_START) {
        fprintf(stderr, "Nie można rozpocząć gry, lobby nie jest gotowe.\n");
        return;
    }

    Game *game = &lobby->game;
    
    printf("Game ID: %d, Lobby ID: %d\n", game->game_id, lobby->lobby_id);
    game_init(game);
    game->status = IN_PROGRESS;

    game->current_turn = CELL_X;

    lobby->status = LOBBY_PLAYING;

    TLVMessage msg;
    StartMessage start_msg;
    start_msg.player_id = lobby->players[lobby->game.current_turn].player_id;
    msg.type = MSG_GAME_START;
    msg.length = sizeof(StartMessage);
    printf("Rozpoczynam grę w lobby %d\n", lobby->lobby_id);
    for (int i = 0; i < lobby->player_count; ++i) {
        char opponent_name[32];
        if (get_name_from_user_id(lobby->players[(i + 1)%2].player_id, opponent_name, sizeof(opponent_name)) < 0) {
            fprintf(stderr, "Nie można pobrać nazwy gracza o ID %d\n", lobby->players[(i + 1)%2].player_id);
            return;
        }
        printf("Gracz %d: %s\n", i, opponent_name);
        memset(start_msg.opponent_name, 0, sizeof(start_msg.opponent_name));
        strncpy(start_msg.opponent_name, opponent_name, sizeof(start_msg.opponent_name) - 1);
        start_msg.opponent_name[sizeof(start_msg.opponent_name) - 1] = '\0';
        memcpy(msg.value, &start_msg, sizeof(start_msg));
        if (send(lobby->players[i].player_fd, &msg, sizeof(msg), 0) < 0) {
            perror("send game start");
            return;
        }
    }
    lobby->status = LOBBY_PLAYING;

    printf("Gra rozpoczęta w lobby %d!\n", lobby->lobby_id);

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

