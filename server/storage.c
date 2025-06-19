#include "storage.h"
#include "auth.h"
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

int file_exists(const char* filename) {
    struct stat buffer;
    return stat(filename, &buffer) == 0;
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

int get_name_from_user_id(int player_id, char* name, size_t name_len) {
    int idx = find_user_by_id(player_id);
    if (idx == -1) return -1;
    strncpy(name, users_auth[idx].login, name_len - 1);
    name[name_len - 1] = '\0';
    return 0;
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