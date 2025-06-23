#include "game.h"
#include "lobby.h"
#include "auth.h"
#include "storage.h"
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

int game_make_move(Move *move, int player_id) {
    // Find the game by player_id
    Game *game = NULL;
    int i;
    for (i = 0; i < MAX_LOBBIES; i++) {
        if (lobbies[i].game.game_id != -1) { // Check if the
            // game is initialized
            if ((lobbies[i].players[0].player_id == player_id || lobbies[i].players[1].player_id == player_id) && lobbies[i].game.status == IN_PROGRESS) {
                game = &lobbies[i].game;
                break;
            }
        }
    }
    if (game == NULL) {
        fprintf(stderr, "Game not found for player ID %d\n", player_id);
        return -1; // Game not found
    }
    if (game->current_turn != (player_id == lobbies[i].players[0].player_id ? CELL_X : CELL_O)) {
        fprintf(stderr, "It's not your turn, player ID %d\n", player_id);
        return -2; // Not your turn
    }
    if (move->row < 0 || move->row >= BOARD_SIZE || move->col < 0 || move->col >= BOARD_SIZE) {
        fprintf(stderr, "Invalid move coordinates: (%d, %d)\n", move->row, move->col);
        return -3; // Invalid move
    }
    if (game->board[move->row][move->col] != CELL_EMPTY)
    {
        fprintf(stderr, "Cell (%d, %d) is already occupied\n", move->row, move->col);
        return -4; // Cell already occupied
    }
    game->board[move->row][move->col] = game->current_turn;
    GameStatus status = game_check_status(game);
    if (status == IN_PROGRESS) {
        game->current_turn = (game->current_turn == CELL_X) ? CELL_O :
                                CELL_X;
    } else {
        // Game finished, set the status
        game->status = status;
        // Save the game result to the database
        if (game->status == WIN_X || game->status == WIN_O) {
            int winner_id = (game->status == WIN_X) ? lobbies[i].players[0].player_id : lobbies[i].players[1].player_id;
            int winner_index = find_user_by_id(winner_id);
            int loser_id = (game->status == WIN_X) ? lobbies[i].players[1].player_id : lobbies[i].players[0].player_id;
            int loser_index = find_user_by_id(loser_id);
            users_auth[winner_index].games_won++;
            users_auth[loser_index].games_lost++;
            users_auth[winner_index].games_played++;
            users_auth[loser_index].games_played++;
            save_users(USER_DB_FILE);
        } else if(game->status == DRAW) {
            int player1_index = find_user_by_id(lobbies[i].players[0].player_id);
            int player2_index = find_user_by_id(lobbies[i].players[1].player_id);
            users_auth[player1_index].games_played++;
            users_auth[player2_index].games_played++;
            save_users(USER_DB_FILE);
        }
    }
    TLVMessage msg;
    msg.type = MSG_RESULT;
    msg.length = sizeof(Game);
    memcpy(msg.value, game, sizeof(Game));
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

int check_if_player_in_game(int player_fd) {
    for (int i = 0; i < MAX_LOBBIES; i++) {
        for (int j = 0; j < lobbies[i].player_count; j++) {
            if (lobbies[i].players[j].player_fd == player_fd) {
                return 1; 
            }
        }
    }
    return 0;
}

void end_user_games(int player_fd) {
    for (int i = 0; i < MAX_LOBBIES; i++) {
        for (int j = 0; j < lobbies[i].player_count; j++) {
            if (lobbies[i].players[j].player_fd == player_fd) {
                printf("Ending game for player %d in lobby %d\n", lobbies[i].players[j].player_id, lobbies[i].game.game_id);
                // Send to the other player that the game has ended and he won
                TLVMessage msg;
                msg.type = MSG_RESULT;
                msg.length = sizeof(Game);
                Game game = lobbies[i].game;
                game.status = (lobbies[i].players[j].player_id == lobbies[i].players[0].player_id) ? WIN_O : WIN_X; // The other player wins
                memcpy(msg.value, &game, sizeof(Game));
                ssize_t total_size = sizeof(msg.type) + sizeof(msg.length) + msg.length;
                for (int k = 0; k < lobbies[i].player_count; k++) {
                    if (lobbies[i].players[k].player_fd != player_fd) {
                        if (send(lobbies[i].players[k].player_fd, &msg, total_size, 0) < 0) {
                            perror("send game result");
                        }
                    }
                }

                if (game.status == WIN_X || game.status == WIN_O) {
                    int winner_id = (game.status == WIN_X) ? lobbies[i].players[0].player_id : lobbies[i].players[1].player_id;
                    int winner_index = find_user_by_id(winner_id);
                    int loser_id = (game.status == WIN_X) ? lobbies[i].players[1].player_id : lobbies[i].players[0].player_id;
                    int loser_index = find_user_by_id(loser_id);
                    users_auth[winner_index].games_won++;
                    users_auth[loser_index].games_lost++;
                    users_auth[winner_index].games_played++;
                    users_auth[loser_index].games_played++;
                    save_users(USER_DB_FILE);
                }

                lobbies[i].players[j].player_fd = -1; 
                lobbies[i].players[j].player_id = -1;
                lobbies[i].player_count = 0;
                if (lobbies[i].player_count == 0) {
                    lobbies[i].game.game_id = -1; 
                    lobbies[i].game.status = IN_PROGRESS;
                    lobbies[i].status = LOBBY_WAITING;
                    lobbies[i].lobby_id = -1;
                }
                return;
            }
        }
    }
}