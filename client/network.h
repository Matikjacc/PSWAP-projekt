#ifndef AUTH_H
#define AUTH_H

bool login(int sockfd, const char* login, const char* password);
void register_account(int sockfd, const char* login, const char* password);

#endif

