#ifndef CLIENT_H
#define CLIENT_H

#include "protocol.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "ui.h"
#include "game.h"

extern UserInfo user;

int get_server_address_from_user(struct sockaddr_in *server_addr);
int connect_with_timeout(int sockfd, struct sockaddr *addr, socklen_t addrlen, int timeout_sec);


#endif