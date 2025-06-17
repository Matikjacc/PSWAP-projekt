#ifndef AUTH_H
#define AUTH_H

#define MAX_USERS 100
#define MAX_NAME_LEN 32
#define MAX_PASS_LEN 32
#define USER_DB_FILE "users.db"

typedef struct {
    char login[MAX_NAME_LEN];
    char password[MAX_PASS_LEN];
    int games_played;
    int games_won;
    int games_lost;
    int player_id;
} User;

extern User users_auth[MAX_USERS];
extern int user_count;

int load_users(const char* filename);
int save_users(const char* filename);
int find_user(const char* login);
int authenticate_user(const char* login, const char* password, int sockfd);
int register_user(const char* login, const char* password, int sockfd);

#endif
