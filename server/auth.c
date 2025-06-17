#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "auth.h"
#include "protocol.h"
#include <netinet/in.h>

User users_auth[MAX_USERS];
int user_count = 0;

static int file_exists(const char* filename) {
    struct stat buffer;
    return stat(filename, &buffer) == 0;
}

int load_users(const char* filename) {
    if (!file_exists(filename)) {
        // Tworzymy pusty plik, jeśli nie istnieje
        FILE* f = fopen(filename, "w");
        if (f) fclose(f);
        return 0;
    }

    FILE* file = fopen(filename, "r");
    if (!file) return -1;

    user_count = 0;

    while (fscanf(file, "%31[^;];%31[^;];%d;%d;%d;%d\n",
                  users_auth[user_count].login,
                  users_auth[user_count].password,
                  &users_auth[user_count].games_played,
                  &users_auth[user_count].games_won,
                  &users_auth[user_count].games_lost,
                  &users_auth[user_count].player_id) == 6) {
        user_count++;
        if (user_count >= MAX_USERS) break;
    }

    fclose(file);
    return 0;
}

int save_users(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) return -1;

    for (int i = 0; i < user_count; i++) {
        fprintf(file, "%s;%s;%d;%d;%d;%d\n",
                users_auth[i].login,
                users_auth[i].password,
                users_auth[i].games_played,
                users_auth[i].games_won,
                users_auth[i].games_lost,
                users_auth[i].player_id);
    }

    fclose(file);
    return 0;
}

int find_user_by_login(const char* login) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users_auth[i].login, login) == 0)
            return i;
    }
    return -1;
}

int find_user_by_id(int player_id) {
    for (int i = 0; i < user_count; i++) {
        if (users_auth[i].player_id == player_id)
            return i;
    }
    return -1;
}

int authenticate_user(const char* login, const char* password, int sockfd) {
    int idx = find_user_by_login(login);
    if (idx == -1) return -1;
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
    return 0;
}
