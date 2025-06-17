#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include "network.h"

typedef enum{
    OPTION_LOGIN = 1,
    OPTION_REGISTER,
    OPTION_FORGOT_PASSWORD,
    OPTION_EXIT
} LoginMenuOption;

typedef enum {
    OPTION_PLAY = 1,
    OPTION_VIEW_STATS,
    OPTION_EXIT_GAME
} MainMenuOption;

bool authenticate(int sockfd);
int get_menu_option(void);
int get_statistics(int sockfd);

#endif