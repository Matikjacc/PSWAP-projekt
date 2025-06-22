#ifndef AUTH_H
#define AUTH_H
#include <stdbool.h>
#include <stddef.h>
#include "client.h"

extern UserInfo user;

bool login(int sockfd, const char* login, const char* password);
void register_account(int sockfd, const char* login, const char* password);
void make_authenticated_message(TLVMessage *msg, MessageType type, const void *value, size_t value_length);

#endif

