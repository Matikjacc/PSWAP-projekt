#ifndef SERVER_DISCOVERY_H
#define SERVER_DISCOVERY_H

#include <netinet/in.h>

// 1 - succes, 0 - timeout, server_addr setter
int discover_server(struct sockaddr_in *server_addr, int timeout_sec);

#endif