#include <stdio.h>
#include <string.h>
#include "game.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

void game_client_init(int sockfd) {
    
    TLVMessage msg;
    msg.type = MSG_JOIN_LOBBY;
    int user_id = user.id;
    memcpy(msg.value, &user_id, sizeof(int));
    msg.length = sizeof(msg.value);

    //send the join lobby message
    ssize_t total_size = sizeof(msg.type) + sizeof(msg.length) + sizeof(msg.value);
    printf("Wysyłanie wiadomości dołączenia do lobby: user_id=%d\n", user_id);
    if (send(sockfd, &msg, total_size, 0) < 0) {
        perror("send join lobby");
        return;
    }
    printf("Wysłano wiadomość dołączenia do lobby.\n");
    // receive the game message
    ssize_t bytes_received = recv(sockfd, &msg, sizeof(msg), 0);
    if (bytes_received < 0) {
        perror("recv join lobby");
        return;
    }
    GameInfo game_info;
    memcpy(&game_info, msg.value, sizeof(GameInfo));

    if(msg.type == MSG_ALL_LOBBIES_FULL) {
        fprintf(stderr, "Wszystkie lobby są pełne.\n");
        return;
    }

    if (msg.type != MSG_JOIN_LOBBY_SUCCESS || msg.length != sizeof(game_info)) {
        fprintf(stderr, "Nieprawidłowa odpowiedź od serwera. Type: %d, Length: %d\n", 
                msg.type, msg.length);
        return;
    }

    if(game_info.status == LOBBY_WAITING){
        TLVMessage game_msg;
        game_msg.type = -1;
        printf("Lobby jest w trakcie oczekiwania na graczy. ID lobby: %d\n", game_info.lobby_id);
        while(game_msg.type != MSG_LOBBY_READY) {
            ssize_t bytes = read(sockfd, &game_msg, sizeof(TLVMessage));
            if (bytes < 0) {
                perror("read lobby ready");
                return;
            } else if (bytes == 0) {
                fprintf(stderr, "Serwer zamknął połączenie.\n");
                return;
            }
            printf("Otrzymano wiadomość od serwera: type=%d, length=%d\n", game_msg.type, game_msg.length);
            if(game_msg.type == MSG_LOBBY_READY) {
                printf("Lobby jest gotowe do gry. ID lobby: %d\n", game_info.lobby_id);
                game_info.status = LOBBY_START;
                start_game(&game_info, sockfd);
                return;
            } else {
                fprintf(stderr, "Nieoczekiwana wiadomość od serwera: %d\n", game_msg.type);
            }
        }
    } else if(game_info.status == LOBBY_PLAYING) {
        printf("Lobby jest w trakcie gry. ID lobby: %d\n", game_info.lobby_id);
    } else if(game_info.status == LOBBY_START){
        printf("Lobby jest gotowe do rozpoczęcia gry. ID lobby: %d\n", game_info.lobby_id);
        start_game(&game_info, sockfd);
        return;
    } else if(game_info.status == LOBBY_FULL) {
        fprintf(stderr, "Lobby jest pełne. Nie można dołączyć.\n");
        return;
    } else {
        fprintf(stderr, "Nieznany status lobby: %d\n", game_info.status);
        return;
    }
    printf("Dołączono do lobby %d. Status: %s\n", game_info.lobby_id, status_to_string(game_info.game.status));
}

void start_game(GameInfo *game_info, int sockfd) {
    Game *game = &game_info->game;
    printf("Game ID: %d, Lobby ID: %d\n", game->game_id, game_info->lobby_id);
    game->current_turn = CELL_X; 
    game->status = IN_PROGRESS;

    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            game->board[i][j] = CELL_EMPTY;
        }
    }

    TLVMessage msg;
    StartMessage start_msg;
    if(read(sockfd, &msg, sizeof(msg)) < 0) {
        perror("read start game");
        return;
    }

    if(msg.type != MSG_GAME_START || msg.length != sizeof(start_msg)) {
        fprintf(stderr, "Nieprawidłowa wiadomość startu gry. Type: %d, Length: %d\n", 
                msg.type, msg.length);
        return;
    }

    memcpy(&start_msg, msg.value, sizeof(start_msg));

    printf("Gra rozpoczęta! ID gry: %d\n", game->game_id);

    game_client_draw(game);

    while (game->status == IN_PROGRESS) {
        if(start_msg.player_id == user.id){
            printf("Twoja tura!\n");
        }else{
            printf("Oczekiwanie na ruch gracza %d...\n", start_msg.player_turn);
        }
        char input[10];
        printf("Tura gracza %c. Wprowadź ruch (np. 1 2): ", game->current_turn == CELL_X ? 'X' : 'O');
        fgets(input, sizeof(input), stdin);

        int row, col;
        if (sscanf(input, "%d %d", &row, &col) != 2 || row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
            printf("Nieprawidłowy ruch. Spróbuj ponownie.\n");
            continue;
        }

        if (game->board[row][col] != CELL_EMPTY) {
            printf("To pole jest już zajęte. Spróbuj ponownie.\n");
            continue;
        }

        game->board[row][col] = game->current_turn;

        // Send move to server
        msg.type = MSG_MOVE;
        msg.length = sizeof(Game);
        memcpy(msg.value, game, sizeof(Game));
        if (send(sockfd, &msg, sizeof(msg), 0) < 0) {
            perror("send move");
            return;
        }

        // Receive updated game state
        if (recv(sockfd, &msg, sizeof(msg), 0) < 0) {
            perror("recv updated game state");
            return;
        }

        if (msg.type != MSG_RESULT || msg.length != sizeof(Game)) {
            fprintf(stderr, "Nieprawidłowa odpowiedź od serwera. Type: %d, Length: %d\n", 
                    msg.type, msg.length);
            return;
        }

        memcpy(game, msg.value, sizeof(Game));
        game_client_draw(game);
    }
    if (game->status == WIN_X) {
        printf("Gratulacje! Gracz X wygrał!\n");
    } else if (game->status == WIN_O) {
        printf("Gratulacje! Gracz O wygrał!\n");
    } else if (game->status == DRAW) {
        printf("Mamy remis!\n");
    } else {
        printf("Gra zakończona w nieznanym stanie.\n");
    }
}


Cell char_to_cell(char c) {
    if (c == 'X') return CELL_X;
    if (c == 'O') return CELL_O;
    return CELL_EMPTY;
}

void game_client_draw(const Game *game) {
    printf("\n  0 1 2\n");
    for (int i = 0; i < BOARD_SIZE; ++i) {
        printf("%d ", i);
        for (int j = 0; j < BOARD_SIZE; ++j) {
            char symbol = '.';
            if (game->board[i][j] == CELL_X) symbol = 'X';
            else if (game->board[i][j] == CELL_O) symbol = 'O';
            printf("%c ", symbol);
        }
        printf("\n");
    }
}

int game_client_update(Game *game, const char *data) {
    if (!data) return 0;

    char board_str[10];
    char turn_char;
    char status_str[32];

    //read from data // CHANGE FORMAT TO TLV
    if (sscanf(data, "%9[^;];%c;%31s", board_str, &turn_char, status_str) != 3)
        return 0;

    for (int i = 0; i < 9; ++i) {
        game->board[i / 3][i % 3] = char_to_cell(board_str[i]);
    }

    game->current_turn = turn_char;
    game->status = string_to_status(status_str);

    return 1;
}

const char* status_to_string(GameStatus status) {
    switch (status) {
        case IN_PROGRESS: return "IN_PROGRESS";
        case DRAW:        return "DRAW";
        case WIN_X:       return "WIN_X";
        case WIN_O:       return "WIN_O";
        default:          return "UNKNOWN";
    }
}

GameStatus string_to_status(const char *str) {
    if (strcmp(str, "IN_PROGRESS") == 0) return IN_PROGRESS;
    if (strcmp(str, "DRAW") == 0)        return DRAW;
    if (strcmp(str, "WIN_X") == 0)       return WIN_X;
    if (strcmp(str, "WIN_O") == 0)       return WIN_O;
    return IN_PROGRESS; // fallback
}