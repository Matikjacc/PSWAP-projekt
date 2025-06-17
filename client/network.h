#ifndef AUTH_H
#define AUTH_H
#include <stdbool.h>
#include "client.h"

extern UserInfo user;

bool login(int sockfd, const char* login, const char* password);
void register_account(int sockfd, const char* login, const char* password);

#endif

