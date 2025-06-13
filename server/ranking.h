#ifndef RANKING_H
#define RANKING_H

#define MAX_USERS 100

struct user {
    char login[32];
    char password[32];
    int games_played;
    int games_won;
    int games_lost;
};

extern struct user users[MAX_USERS];  // Declare as extern
void sort_users_by_performance(struct user *users, int user_count);
char* get_statistics(void);

#endif