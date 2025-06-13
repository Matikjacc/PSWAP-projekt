#include <stdio.h>
#include "ranking.h"
#include <string.h>
#include <unistd.h>

struct user users[MAX_USERS];

static float get_win_ratio(struct user *user) {
    return user->games_won / (float)(user->games_played > 0 ? user->games_played : 1);
}

static int partition(struct user *users, int low, int high) {
    float pivot = get_win_ratio(&users[high]);
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (get_win_ratio(&users[j]) > pivot) {
            i++;
            struct user temp = users[i];
            users[i] = users[j];
            users[j] = temp;
        }
    }
    struct user temp = users[i + 1];
    users[i + 1] = users[high];
    users[high] = temp;
    return i + 1;
}

static void quicksort(struct user *users, int low, int high) {
    if (low < high) {
        int pi = partition(users, low, high);
        quicksort(users, low, pi - 1);
        quicksort(users, pi + 1, high);
    }
}

void sort_users_by_performance(struct user *users, int user_count) {
    quicksort(users, 0, user_count - 1);
}

char* get_statistics() {
    FILE *file = fopen("users.db", "r");
    if (!file) {
        perror("Failed to open users.db");
        return NULL;
    }

    static char ranking_buffer[4096]; 
    char temp_buffer[256];
    
    memset(ranking_buffer, 0, sizeof(ranking_buffer)); 

    int user_count = 0;
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

    sort_users_by_performance(users, user_count);

    int display_count = user_count > 20 ? 20 : user_count;
    for (int i = 0; i < display_count; i++) {
        float win_ratio = users[i].games_won / (float)(users[i].games_played > 0 ? users[i].games_played : 1);
        snprintf(temp_buffer, sizeof(temp_buffer), 
                "#%d %s - Games: %d, Won: %d, Lost: %d (%.2f%%)\n",
                i + 1,
                users[i].login,
                users[i].games_played,
                users[i].games_won,
                users[i].games_lost,
                win_ratio * 100);
        strcat(ranking_buffer, temp_buffer);
    }

    return ranking_buffer;
}