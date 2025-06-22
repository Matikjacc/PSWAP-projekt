#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "auth.h"
#include "protocol.h"
#include "storage.h"
#include <netinet/in.h>

User users_auth[MAX_USERS];
int user_count = 0;

LoggedUser logged_users[MAX_USERS];
int logged_user_count = 0;

int authenticate_user(const char* login, const char* password, int sockfd) {
    int idx = find_user_by_login(login);
    if (idx == -1) return -1;

    if (is_user_logged_in(login)) {
        fprintf(stderr, "Użytkownik %s jest już zalogowany.\n", login);
        TLVMessage msg;
        msg.type = MSG_LOGIN_ALREADY_LOGGED_IN;
        msg.length = 0;
        ssize_t total_size = sizeof(msg.type) + sizeof(msg.length);
        if (send(sockfd, &msg, total_size, 0) < 0) {
            perror("send login failure");
            return -1;
        }
        return -2;
    }

    if (strcmp(users_auth[idx].password, password) == 0){
        TLVMessage msg;
        msg.type = MSG_LOGIN_SUCCESS;
        msg.length = sizeof(UserInfo);
        UserInfo user_info;
        user_info.id = users_auth[idx].player_id;
        strncpy(user_info.login, users_auth[idx].login, sizeof(user_info.login) - 1);
        user_info.login[sizeof(user_info.login) - 1] = '\0';
        memcpy(msg.value, &user_info, sizeof(UserInfo));
        ssize_t total_size = sizeof(msg.type) + sizeof(msg.length) + msg.length;
        if (send(sockfd, &msg, total_size, 0) < 0) {
            perror("send login success");
            return -1;
        }

        if (add_logged_user(user_info.id, sockfd, login) < 0) {
            fprintf(stderr, "Lista zalogowanych pełna, nie można dodać użytkownika.\n");
            return -1;
        }
        return 0;
    }
    return -1;
}

int register_user(const char* login, const char* password, int sockfd) {
    if (user_count >= MAX_USERS) return -1;
    if (find_user_by_login(login) != -1) return -2;  // Login zajęty
    int user_id;
    while (find_user_by_id(user_id = rand()) != -1);
    strncpy(users_auth[user_count].login, login, MAX_NAME_LEN);
    strncpy(users_auth[user_count].password, password, MAX_PASS_LEN);
    users_auth[user_count].games_played = 0;
    users_auth[user_count].games_won = 0;
    users_auth[user_count].games_lost = 0;
    users_auth[user_count].player_id = user_id;
    user_count++;

    if(save_users(USER_DB_FILE) < 0){
        fprintf(stderr, "Błąd podczas zapisywania użytkowników do pliku.\n");
        return -3;
    }

    TLVMessage msg;
    msg.type = MSG_REGISTER_SUCCESS;
    msg.length = sizeof(UserInfo);
    UserInfo user_info;
    user_info.id = user_id;
    strncpy(user_info.login, login, sizeof(user_info.login) - 1);
    user_info.login[sizeof(user_info.login) - 1] = '\0';
    memcpy(msg.value, &user_info, sizeof(UserInfo));
    ssize_t total_size = sizeof(msg.type) + sizeof(msg.length) + msg.length;
    if (send(sockfd, &msg, total_size, 0) < 0) {
        perror("send register success");
        return -4;
    }

    if (add_logged_user(user_info.id, sockfd, login) < 0) {
        fprintf(stderr, "Lista zalogowanych pełna, nie można dodać użytkownika.\n");
        return -1;
    }

    return 0;
}

int is_user_logged_in(const char* login) {
    for (int i = 0; i < logged_user_count; i++) {
        if (strcmp(logged_users[i].login, login) == 0) {
            return 1; // jest już zalogowany
        }
    }
    return 0;
}

int add_logged_user(int player_id, int sockfd, const char* login) {
    if (logged_user_count >= MAX_USERS) return -1;
    logged_users[logged_user_count].player_id = player_id;
    logged_users[logged_user_count].socket_fd = sockfd;
    strncpy(logged_users[logged_user_count].login, login, MAX_NAME_LEN - 1);
    logged_users[logged_user_count].login[MAX_NAME_LEN - 1] = '\0';
    logged_user_count++;
    return 0;
}

int remove_logged_user_by_fd(int sockfd) {
    for (int i = 0; i < logged_user_count; ++i) {
        if (logged_users[i].socket_fd == sockfd) {
            // Przesuń resztę użytkowników o jedno miejsce w lewo
            for (int j = i; j < logged_user_count - 1; ++j) {
                logged_users[j] = logged_users[j + 1];
            }
            logged_user_count--;
            return 0; // Sukces
        }
    }
    return -1; // Nie znaleziono
}

char* get_active_users_list() {
    static char active_users_buffer[1024];
    char temp_buffer[64];
    
    memset(active_users_buffer, 0, sizeof(active_users_buffer));

    if (logged_user_count == 0) {
        snprintf(active_users_buffer, sizeof(active_users_buffer), "Brak aktywnych graczy.\n");
        return active_users_buffer;
    }

    snprintf(active_users_buffer, sizeof(active_users_buffer), "Aktywni gracze (%d):\n", logged_user_count);

    for (int i = 0; i < logged_user_count; i++) {
        snprintf(temp_buffer, sizeof(temp_buffer), "- %s\n", logged_users[i].login);
        strncat(active_users_buffer, temp_buffer, sizeof(active_users_buffer) - strlen(active_users_buffer) - 1);
    }

    return active_users_buffer;
}