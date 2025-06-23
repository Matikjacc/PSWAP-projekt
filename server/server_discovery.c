#define _GNU_SOURCE
#include "server_discovery.h"
#include "config.h"
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>

int init_discovery_socket() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        syslog(LOG_ERR, "socket: %s", strerror(errno));
        return -1;
    }

    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(MULTICAST_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        syslog(LOG_ERR, "bind: %s", strerror(errno));
        close(sock);
        return -1;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
        syslog(LOG_ERR, "setsockopt IP_ADD_MEMBERSHIP: %s", strerror(errno));
        close(sock);
        return -1;
    }

    // Ustaw socket jako nieblokujący
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    return sock;
}

void handle_discovery_message(int sock_fd) {
    TLVMessage msg;
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);

    ssize_t len = recvfrom(sock_fd, &msg, sizeof(msg), 0,
                           (struct sockaddr *)&client_addr, &addrlen);

    if (len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;  // Nie ma danych
        }
        syslog(LOG_ERR, "recvfrom discovery: %s", strerror(errno));
        return;
    }

    // Sprawdzenie poprawności wiadomości
    if (msg.type == MSG_SERVER_DISCOVERY) {
        syslog(LOG_INFO, "[Discovery] TLV zapytanie od %s:%d",
            inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port));

        // Przygotuj wiadomość odpowiedzi
        TLVMessage response;
        response.type = MSG_SERVER_DISCOVERY_RESPONSE;
        uint16_t port_net = htons(PORT);
        response.length = sizeof(port_net);
        memcpy(response.value, &port_net, sizeof(port_net));

        sendto(sock_fd, &response, sizeof(response.type) + sizeof(response.length) + response.length, 0,
               (struct sockaddr *)&client_addr, addrlen);
    }
}
