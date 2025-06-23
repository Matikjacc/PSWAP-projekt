#ifndef DISCOVERY_H
#define DISCOVERY_H

#define BUFFER_SIZE 1024

int init_discovery_socket();
void handle_discovery_message(int sock_fd);

#endif
