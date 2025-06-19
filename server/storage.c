#include "storage.h"
#include "auth.h"
#include <string.h>

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