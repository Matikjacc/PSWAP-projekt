#ifndef CLIENT_H
#define CLIENT_H

#include "protocol.h"
#include "ui.h"
#include "game.h"

extern UserInfo user;

void get_active_players(int sockfd);

#endif