#include <stdio.h>
#include <string.h>
#include "game.h"
#include "../common/protocol.h"
#include <netinet/in.h>
#include <stdlib.h>

void game_client_init(int sockfd) {
    
    TLVMessage msg;
    msg.type = MSG_JOIN_LOBBY;
    msg.length = 0;

    //send the join lobby message
    ssize_t total_size = sizeof(msg.type) + sizeof(msg.length);
    if (send(sockfd, &msg, total_size, 0) < 0) {
        perror("send join lobby");
        return;
    }

    // receive the game message
    ssize_t bytes_received = recv(sockfd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
    if (bytes_received < 0) {
        if(msg.type == MSG_ALL_LOBBIES_FULL) {
            fprintf(stderr, "Wszystkie lobby są pełne.\n");
        } else {
            perror("recv join lobby");
        }
        return;
    }
    
    Game *game = malloc(sizeof(Game));
    memset(game, 0, sizeof(Game));
    game->game_id = -1; // Initialize game ID to an invalid value
    game->current_turn = CELL_X; // Start with player X
    game->status = IN_PROGRESS; // Game starts in progress

    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            game->board[i][j] = CELL_EMPTY;
    game->current_turn = 'X';
    game->status = IN_PROGRESS;
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
    printf("Status: %s | Tura: %c\n\n", status_to_string(game->status), game->current_turn);
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