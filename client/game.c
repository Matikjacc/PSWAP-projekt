#include <stdio.h>
#include <string.h>
#include "game.h"
#include "client.h"
#include "network.h"
#include "client.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

void game_client_init(int sockfd)
{
    TLVMessage msg;
    msg.type = MSG_JOIN_LOBBY;
    msg.length = sizeof(int); // tylko player_id
    memcpy(msg.value, &user.id, sizeof(int));

    ssize_t total_size = sizeof(msg.type) + sizeof(msg.length) + msg.length;
    if (send(sockfd, &msg, total_size, 0) < 0) {
        perror("send join lobby");
        return;
    }
    printf("Wysłano wiadomość dołączenia do lobby: user_id=%d\n", user.id);
    // receive the game message
    ssize_t header_bytes = recv(sockfd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
    if (header_bytes < 0){
        perror("recv join lobby header");
        return;
    } else if (header_bytes == 0) {
        fprintf(stderr, "Serwer zamknął połączenie.\n");
        close(sockfd);
        exit(0);
        return;
    }
    printf("Otrzymano wiadomość od serwera: type=%d, length=%d\n", msg.type, msg.length);
    ssize_t value_bytes = recv(sockfd, msg.value, msg.length, 0);
    if (value_bytes < 0){
        perror("recv join lobby value");
        return;
    }

    GameInfo game_info;
    memcpy(&game_info, msg.value, sizeof(GameInfo));

    if (msg.type == MSG_ALL_LOBBIES_FULL)
    {
        fprintf(stderr, "Wszystkie lobby są pełne.\n");
        sleep(3);
        return;
    }

    if (msg.type == MSG_JOIN_LOBBY_FAIL)
    {
        fprintf(stderr, "Nie możesz dołączyć do lobby, w którym jesteś.\n");
        sleep(3);
        return;
    }

    if (msg.type != MSG_JOIN_LOBBY_SUCCESS || msg.length != sizeof(game_info))
    {
        fprintf(stderr, "Nieprawidłowa odpowiedź od serwera. Type: %d, Length: %d\n",
                msg.type, msg.length);
        sleep(3);
        return;
    }

    if (game_info.status == LOBBY_WAITING)
    {
        TLVMessage game_msg;
        game_msg.type = -1;
        printf("Lobby jest w trakcie oczekiwania na graczy. ID lobby: %d\n", game_info.lobby_id);
        while (game_msg.type != MSG_LOBBY_READY)
        {
            ssize_t bytes = read(sockfd, &game_msg, sizeof(TLVMessage));
            if (bytes < 0)
            {
                perror("read lobby ready");
                return;
            }
            else if (bytes == 0)
            {
                fprintf(stderr, "Serwer zamknął połączenie.\n");
                return;
            }
            printf("Otrzymano wiadomość od serwera: type=%d, length=%d\n", game_msg.type, game_msg.length);
            if (game_msg.type == MSG_LOBBY_READY)
            {
                printf("Lobby jest gotowe do gry. ID lobby: %d\n", game_info.lobby_id);
                game_info.status = LOBBY_START;
                start_game(&game_info, sockfd);
                return;
            }
            else
            {
                fprintf(stderr, "Nieoczekiwana wiadomość od serwera: %d\n", game_msg.type);
            }
        }
    }
    else if (game_info.status == LOBBY_PLAYING)
    {
        printf("Lobby jest w trakcie gry. ID lobby: %d\n", game_info.lobby_id);
    }
    else if (game_info.status == LOBBY_START)
    {
        printf("Lobby jest gotowe do rozpoczęcia gry. ID lobby: %d\n", game_info.lobby_id);
        start_game(&game_info, sockfd);
        return;
    }
    else if (game_info.status == LOBBY_FULL)
    {
        fprintf(stderr, "Lobby jest pełne. Nie można dołączyć.\n");
        return;
    }
    else
    {
        fprintf(stderr, "Nieznany status lobby: %d\n", game_info.status);
        return;
    }
    printf("Dołączono do lobby %d.", game_info.lobby_id);
}

void start_game(GameInfo *game_info, int sockfd)
{
    Game *game = &game_info->game;
    printf("Game ID: %d, Lobby ID: %d\n", game->game_id, game_info->lobby_id);
    game->current_turn = CELL_X;
    game->status = IN_PROGRESS;
    Cell myturn;

    for (int i = 0; i < BOARD_SIZE; ++i)
    {
        for (int j = 0; j < BOARD_SIZE; ++j)
        {
            game->board[i][j] = CELL_EMPTY;
        }
    }

    TLVMessage msg;
    StartMessage start_msg;
    if (read(sockfd, &msg, sizeof(msg)) < 0)
    {
        perror("read start game");
        return;
    }

    if (msg.type != MSG_GAME_START || msg.length != sizeof(start_msg))
    {
        fprintf(stderr, "Nieprawidłowa wiadomość startu gry. Type: %d, Length: %d\n",
                msg.type, msg.length);
        return;
    }

    memcpy(&start_msg, msg.value, sizeof(start_msg));
    // clear screen
    printf("\033[2J\033[H");
    printf("Gra rozpoczęta! ID gry: %d\n", game->game_id);

    game_client_draw(game);

    if(start_msg.player_id == user.id)
    {
        myturn = CELL_X;
    }
    else
    {
        myturn = CELL_O;
    }

    while (game->status == IN_PROGRESS)
    {
        printf("Current turn: %c\n", game->current_turn == CELL_X ? 'X' : 'O');
        if (myturn == game->current_turn)
        {
            printf("Twoja tura!\n");
            char input[10];
            printf("Ruchy są w formacie: <wiersz> <kolumna>\n");
        get_player_move:
            int row, col;
            short move_correct = 0;
            do
            {
                printf("Wprowadź ruch (np. 1 2): ");
                if (!fgets(input, sizeof(input), stdin)) {
                    printf("Błąd wejścia.\n");
                    continue;
                }
                if (sscanf(input, "%d %d", &row, &col) != 2) {
                    printf("Nieprawidłowy format. Wprowadź ruch ponownie (np. 1 2): ");
                    continue;
                }
                if(row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
                {
                    printf("Nieprawidłowe współrzędne. Wprowadź ruch ponownie (np. 1 2): ");
                    continue;
                }
                move_correct = 1;
            }while(move_correct == 0);

            if (game->board[row][col] != CELL_EMPTY)
            {
                printf("To pole jest już zajęte. Spróbuj ponownie.\n");
                goto get_player_move;
            }

            game->board[row][col] = game->current_turn;

            Move move;
            AuthenicatedMessage auth_msg;
            move.row = row;
            move.col = col;
            msg.type = MSG_MOVE;
            msg.length = sizeof(int) + sizeof(move.row) + sizeof(move.col);
            auth_msg.player_id = user.id;
            memcpy(auth_msg.value, &move, sizeof(Move));
            memcpy(msg.value, &auth_msg, sizeof(auth_msg.player_id) + sizeof(move));
            if (send(sockfd, &msg, sizeof(msg.type) + sizeof(msg.length) + msg.length, 0) < 0)
            {
                perror("send move");
                return;
            }
        }
        else
        {
            printf("Oczekiwanie na ruch gracza %s...\n", start_msg.opponent_name);
        }

        if (recv(sockfd, &msg, sizeof(msg), 0) < 0)
        {
            perror("recv updated game state");
            return;
        }

        if (msg.type != MSG_RESULT || msg.length != sizeof(Game))
        {
            fprintf(stderr, "Nieprawidłowa odpowiedź od serwera. Type: %d, Length: %d\n",
                    msg.type, msg.length);
            return;
        }

        memcpy(game, msg.value, sizeof(Game));
        //clear screen
        printf("\033[2J\033[H");
        game_client_draw(game);
    }
    if (game->status == WIN_X)
    {
        printf("Gratulacje! Gracz X wygrał!\n");
    }
    else if (game->status == WIN_O)
    {
        printf("Gratulacje! Gracz O wygrał!\n");
    }
    else if (game->status == DRAW)
    {
        printf("Mamy remis!\n");
    }
    else
    {
        printf("Gra zakończona w nieznanym stanie.\n");
    }
    sleep(5);
}

void game_client_draw(const Game *game)
{
    printf("\n  0 1 2\n");
    for (int i = 0; i < BOARD_SIZE; ++i)
    {
        printf("%d ", i);
        for (int j = 0; j < BOARD_SIZE; ++j)
        {
            char symbol = '.';
            if (game->board[i][j] == CELL_X)
                symbol = 'X';
            else if (game->board[i][j] == CELL_O)
                symbol = 'O';
            printf("%c ", symbol);
        }
        printf("\n");
    }
}