#ifndef STORAGE_H
#define STORAGE_H

#include <stdlib.h>

#define USER_DB_FILE "users.db"

int load_users();
int save_users();
int file_exists();
int find_user_by_login(const char* login);
int find_user_by_id(int player_id);
int get_name_from_user_id(int player_id, char* name, size_t name_len);

#endif