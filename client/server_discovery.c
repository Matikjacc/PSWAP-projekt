
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "server_discovery.h"
#include "protocol.h"

#define DISCOVERY_PORT 5555
#define MULTICAST_GROUP "239.1.4.8"
#define BUFFER_SIZE 1024

int discover_server(struct sockaddr_in *server_addr, int timeout_sec) {
    int sockfd;
    struct sockaddr_in multicast_addr;
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    TLVMessage request, response;
    int ret;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 0;
    }

    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(sockfd);
        return 0;
    }

    struct timeval tv = { .tv_sec = timeout_sec, .tv_usec = 0 };
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        close(sockfd);
        return 0;
    }

    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
    multicast_addr.sin_port = htons(DISCOVERY_PORT);

    request.type = MSG_SERVER_DISCOVERY;
    request.length = 0; // Brak danych w treści
    memset(request.value, 0, MAX_PAYLOAD);

    if (sendto(sockfd, &request, sizeof(request.type) + sizeof(request.length), 0,
               (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0) {
        perror("sendto");
        close(sockfd);
        return 0;
    }

    ret = recvfrom(sockfd, &response, sizeof(response), 0,
                   (struct sockaddr *)&from_addr, &from_len);
    if (ret < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            printf("Timeout oczekiwania na odpowiedź serwera\n");
        } else {
            perror("recvfrom");
        }
        close(sockfd);
        return 0;
    }

    if (response.type != MSG_SERVER_DISCOVERY_RESPONSE || response.length != sizeof(uint16_t)) {
        fprintf(stderr, "Nieprawidłowa odpowiedź (typ: %d, długość: %d)\n",
                response.type, response.length);
        close(sockfd);
        return 0;
    }

    // Parse port
    uint16_t port;
    memcpy(&port, response.value, sizeof(port));
    port = ntohs(port);

    printf("[Wyszukiwanie serwera] Otrzymano odpowiedź od %s: port %d\n",
           inet_ntoa(from_addr.sin_addr), port);

    // Fill server_addr
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr = from_addr.sin_addr;
    server_addr->sin_port = htons(port);

    close(sockfd);
    return 1;
}