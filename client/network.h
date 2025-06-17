#ifndef AUTH_H
#define AUTH_H
#include "client.h"
#include <stdbool.h>

UserInfo user;

bool login(int sockfd, const char* login, const char* password);
void register_account(int sockfd, const char* login, const char* password);

#endif

