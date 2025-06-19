#ifndef STORAGE_H
#define STORAGE_H

#include <stdlib.h>

#define USER_DB_FILE "users.db"

int load_users(const char* filename);
int save_users(const char* filename);
int find_user(const char* login);
int file_exists(const char* filename);
int find_user_by_login(const char* login);
int find_user_by_id(int player_id);
int get_name_from_user_id(int player_id, char* name, size_t name_len);

#endif