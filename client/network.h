#ifndef AUTH_H
#define AUTH_H

void login(int sockfd, const char* login, const char* password);
void register_account(int sockfd, const char* login, const char* password);

#endif

