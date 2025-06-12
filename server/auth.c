#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "auth.h"

User users[MAX_USERS];
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

    while (fscanf(file, "%31[^;];%31[^;];%d;%d;%d\n",
                  users[user_count].login,
                  users[user_count].password,
                  &users[user_count].games_played,
                  &users[user_count].games_won,
                  &users[user_count].games_lost) == 5) {
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
        fprintf(file, "%s;%s;%d;%d;%d\n",
                users[i].login,
                users[i].password,
                users[i].games_played,
                users[i].games_won,
                users[i].games_lost);
    }

    fclose(file);
    return 0;
}

int find_user(const char* login) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].login, login) == 0)
            return i;
    }
    return -1;
}

int authenticate_user(const char* login, const char* password) {
    int idx = find_user(login);
    if (idx == -1) return -1;
    if (strcmp(users[idx].password, password) == 0)
        return idx;
    return -1;
}

int register_user(const char* login, const char* password) {
    if (user_count >= MAX_USERS) return -1;
    if (find_user(login) != -1) return -2;  // Login zajęty

    strncpy(users[user_count].login, login, MAX_NAME_LEN);
    strncpy(users[user_count].password, password, MAX_PASS_LEN);
    users[user_count].games_played = 0;
    users[user_count].games_won = 0;
    users[user_count].games_lost = 0;
    user_count++;

    return save_users(USER_DB_FILE);
}
