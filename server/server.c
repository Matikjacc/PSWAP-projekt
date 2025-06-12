#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>

#include "auth.h"
#include "protocol.h"

#define PORT 1234
#define BACKLOG 10
#define MAX_EVENTS 10

// Make socket non-blocking
static int make_socket_non_blocking(int sfd) {
    int flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }
    if (fcntl(sfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }
    return 0;
}

void handle_client_message(int client_fd) {
    TLVMessage msg;
    
    ssize_t header_bytes = read(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length));
    if (header_bytes == 0) {
        printf("Klient się rozłączył.\n");
        close(client_fd);
        return;
    } else if (header_bytes < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Błąd odczytu nagłówka");
            close(client_fd);
        }
        return;
    }

    if (msg.length > MAX_PAYLOAD) {
        fprintf(stderr, "Zbyt długi payload (%u bajtów)\n", msg.length);
        close(client_fd);
        return;
    }

    ssize_t value_bytes = read(client_fd, msg.value, msg.length);
    if (value_bytes != msg.length) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            fprintf(stderr, "Nieprawidłowa liczba bajtów od klienta\n");
            close(client_fd);
        }
        return;
    }

    printf("Odebrano wiadomość: typ=%u, długość=%u\n", msg.type, msg.length);
    
    if(msg.type == MSG_LOGIN) {
        const char* login = msg.value;
        const char* password = msg.value + strlen(login) + 1;
        printf("Login: \"%s\", Hasło: \"%s\"\n", login, password);
        if (authenticate_user(login, password) == 0) {
            printf("Użytkownik \"%s\" zalogowany pomyślnie.\n", login);
            msg.type = MSG_LOGIN_SUCCESS;
            msg.length = 0;
            send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
        } else {
            printf("Błąd logowania dla użytkownika \"%s\".\n", login);
            msg.type = MSG_LOGIN_FAILURE;
            msg.length = 0;
            send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
        }
    } else if (msg.type == MSG_REGISTER) {
        const char* login = msg.value;
        const char* password = msg.value + strlen(login) + 1;
        if (register_user(login, password) == 0) {
            printf("Użytkownik \"%s\" zarejestrowany pomyślnie.\n", login);
            msg.type = MSG_REGISTER_SUCCESS;
            msg.length = 0;
            send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
        } else {
            printf("Błąd rejestracji użytkownika \"%s\".\n", login);
            msg.type = MSG_REGISTER_FAILURE;
            msg.length = 0;
            send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
        }
    } else {
        fprintf(stderr, "Nieznany typ wiadomości: %u\n", msg.type);
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct epoll_event ev, events[MAX_EVENTS];
    int epollfd;

    if (load_users(USER_DB_FILE) < 0) {
        fprintf(stderr, "Nie udało się załadować bazy danych użytkowników.\n");
        return 1;
    }

    printf("Załadowano %d użytkowników z pliku.\n", user_count);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    make_socket_non_blocking(server_fd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    printf("Serwer nasłuchuje na porcie %d...\n", PORT);

    while (1) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                // Nowe połączenie
                client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd == -1) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("accept");
                    }
                    continue;
                }

                make_socket_non_blocking(client_fd);
                
                printf("Połączono z klientem: %s:%d\n",
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port));

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("epoll_ctl");
                    close(client_fd);
                    continue;
                }
            } else {
                // Obsługa danych od klienta
                handle_client_message(events[n].data.fd);
            }
        }
    }

    close(server_fd);
    close(epollfd);
    return 0;
}
