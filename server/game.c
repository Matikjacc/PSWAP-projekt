#include "game.h"
#include "lobby.h"

void game_init(Game *game) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            game->board[i][j] = CELL_EMPTY;
        }
    }
    game->current_turn = CELL_X;
    game->status = IN_PROGRESS;
}

int game_make_move(Game *game, int row, int col) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
        return 0;

    if (game->board[row][col] != CELL_EMPTY || game->status != IN_PROGRESS)
        return 0;

    game->board[row][col] = game->current_turn;

    game->status = game_check_status(game);

    if (game->status == IN_PROGRESS) {
        game->current_turn = (game->current_turn == CELL_X) ? CELL_O : CELL_X;
    }

    return 1;
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