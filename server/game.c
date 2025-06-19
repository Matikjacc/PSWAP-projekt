#include "game.h"
#include "lobby.h"
#include "../common/protocol.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

void game_init(Game *game) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            game->board[i][j] = CELL_EMPTY;
        }
    }
    game->current_turn = CELL_X;
    game->status = IN_PROGRESS;
}

int game_make_move(Game *game) {
    // Check if the game is still in progress
    if (game->status != IN_PROGRESS) {
        return -1; // Game is not in progress
    }
    // Check if the move is valid
    if (game->current_turn != CELL_X && game->current_turn != CELL_O) {
        return -2; // Invalid turn
    }
    // Find lobby of the game
    printf("Game is still in progress, current turn: %c\n", game->current_turn);
    int i;
    short move_made = 0;
    for(i = 0; i < MAX_LOBBIES; i++) {
        if (lobbies[i].game.game_id == game->game_id) {
            for (int row = 0; row < BOARD_SIZE; row++) {
                for (int col = 0; col < BOARD_SIZE; col++) {
                    if (lobbies[i].game.board[row][col] == CELL_EMPTY && game->board[row][col] == game->current_turn) {
                        game->board[row][col] = game->current_turn;
                        game->current_turn = (game->current_turn == CELL_X) ? CELL_O : CELL_X;
                        game->status = game_check_status(game);
                        printf("Move made at (%d, %d) by player %c\n", row, col, game->current_turn == CELL_X ? 'X' : 'O');
                        move_made = 1;
                        break;
                    }
                }
            }
            break;
        }
    }
    if  (!move_made) {
        TLVMessage msg;
        msg.type = MSG_WRONG_MOVE;
        msg.length = 0;
        if (send(lobbies[i].players[lobbies->game.current_turn].player_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0) < 0) {
            perror("send wrong move");
            return -2; 
        }
    }
    if (i == MAX_LOBBIES) {
        return -3;
    }
    TLVMessage msg;
    msg.type = MSG_RESULT;
    msg.length = sizeof(Game);
    memcpy(msg.value, game, sizeof(Game));
    memcpy(lobbies[i].game.board, game->board, sizeof(game->board));
    ssize_t total_size = sizeof(msg.type) + sizeof(msg.length) + msg.length;
    for (int j = 0; j < lobbies[i].player_count; j++) {
        if (send(lobbies[i].players[j].player_fd, &msg, total_size, 0) < 0) {
            perror("send game result");
            return -4; // Error sending result 
        }
    }

}

GameStatus game_check_status(Game *game) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        // Rows
        if (game->board[i][0] != CELL_EMPTY &&
            game->board[i][0] == game->board[i][1] &&
            game->board[i][1] == game->board[i][2]) {
            return (game->board[i][0] == CELL_X) ? WIN_X : WIN_O;
        }
        // Columns
        if (game->board[0][i] != CELL_EMPTY &&
            game->board[0][i] == game->board[1][i] &&
            game->board[1][i] == game->board[2][i]) {
            return (game->board[0][i] == CELL_X) ? WIN_X : WIN_O;
        }
    }

    // Diagonals
    // Main diagonal
    int main_diag_win = 1;
    for (int i = 1; i < BOARD_SIZE; i++) {
        if (game->board[i][i] != game->board[0][0] || game->board[0][0] == CELL_EMPTY) {
            main_diag_win = 0;
            break;
        }
    }
    if (main_diag_win) {
        return (game->board[0][0] == CELL_X) ? WIN_X : WIN_O;
    }

    // Anti-diagonal
    int anti_diag_win = 1;
    for (int i = 1; i < BOARD_SIZE; i++) {
        if (game->board[i][BOARD_SIZE - i - 1] != game->board[0][BOARD_SIZE - 1] || game->board[0][BOARD_SIZE - 1] == CELL_EMPTY) {
            anti_diag_win = 0;
            break;
        }
    }
    if (anti_diag_win) {
        return (game->board[0][BOARD_SIZE - 1] == CELL_X) ? WIN_X : WIN_O;
    }

    // Check for draw
    int full = 1;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] == CELL_EMPTY) {
                full = 0;
                break;
            }
        }
    }

    return full ? DRAW : IN_PROGRESS;
}

Cell game_current_turn(const Game *game) {
    return game->current_turn;
}

Cell game_get_cell(const Game *game, int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
        return CELL_EMPTY;
    return game->board[row][col];
}