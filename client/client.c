#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <netdb.h>

#include "client.h"
#include "server_discovery.h"

int get_server_address_from_user(struct sockaddr_in *server_addr) {
    char input_host[256];
    int input_port;

    while (1) {
        printf("Proszę podać adres IP lub nazwę domenową serwera: ");
        if (fgets(input_host, sizeof(input_host), stdin) == NULL) {
            return 0;
        }
        input_host[strcspn(input_host, "\n")] = '\0';  // remove \n

        printf("Proszę podać port serwera: ");
        if (scanf("%d", &input_port) != 1) {
            fprintf(stderr, "Niepoprawny port.\n");
            while(getchar() != '\n'); // clear stdin
            continue;
        }
        while(getchar() != '\n'); // clear stdin

        if (input_port <= 0 || input_port > 65535) {
            fprintf(stderr, "Niepoprawny port. Spróbuj ponownie.\n");
            continue;
        }

        memset(server_addr, 0, sizeof(*server_addr));
        server_addr->sin_family = AF_INET;
        server_addr->sin_port = htons(input_port);

        // First try to parse as IPv4 address
        if (inet_pton(AF_INET, input_host, &server_addr->sin_addr) == 1) {
            // Poprawny adres IPv4
            return 1;
        } 

        // Else try to resolve hostname
        struct addrinfo hints = {0}, *res = NULL;
        hints.ai_family = AF_INET; // IPv4
        hints.ai_socktype = SOCK_STREAM;
        
        int err = getaddrinfo(input_host, NULL, &hints, &res);
        if (err != 0) {
            fprintf(stderr, "Nie można rozwiązać nazwy '%s': %s\n", input_host, gai_strerror(err));
            continue;
        }

        // Copy resolved address to server_addr
        struct sockaddr_in *addr_in = (struct sockaddr_in *) res->ai_addr;
        server_addr->sin_addr = addr_in->sin_addr;
        freeaddrinfo(res);
        return 1;
    }
}

int connect_with_timeout(int sockfd, struct sockaddr *addr, socklen_t addrlen, int timeout_sec) {
    // 1. set socket non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }

    // connect
    int res = connect(sockfd, addr, addrlen);
    if (res == 0) {
        // połączenie natychmiast udane
        fcntl(sockfd, F_SETFL, flags); // rfert to blocking mode
        return 0;
    } else if (errno != EINPROGRESS) {
        perror("connect");
        fcntl(sockfd, F_SETFL, flags);
        return -1;
    }

    // zestaw sprawdzania gotowych do zapisu deskryptorów
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(sockfd, &writefds);

    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;

    res = select(sockfd + 1, NULL, &writefds, NULL, &tv);
    if (res == 0) {
        // timeout
        fprintf(stderr, "Nie udało się połączyć po %d sekundach\n", timeout_sec);
        fcntl(sockfd, F_SETFL, flags);
        return -1;
    } else if (res < 0) {
        perror("select");
        fcntl(sockfd, F_SETFL, flags);
        return -1;
    }

    // connect success
    int so_error;
    socklen_t len = sizeof(so_error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
        perror("getsockopt");
        fcntl(sockfd, F_SETFL, flags);
        return -1;
    }

    fcntl(sockfd, F_SETFL, flags); // revert to blocking mode

    if (so_error != 0) {
        fprintf(stderr, "Błąd połączenia: %s\n", strerror(so_error));
        return -1;
    }

    return 0;
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    printf("Oczekiwanie na odkrycie serwera...\n");
    if (!discover_server(&server_addr, 5)) {
        fprintf(stderr, "Nie udało się odnaleźć serwera przez discovery.\n");
        if (!get_server_address_from_user(&server_addr)) {
            fprintf(stderr, "Błąd odczytu adresu serwera. Kończenie programu.\n");
            exit(EXIT_FAILURE);
        }
    }
    else {
        printf("Serwer odkryty przez multicast: %s:%d\n",
               inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("Łączenie z serwerem %s:%d\n",
           inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    if (connect_with_timeout(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr), 30) < 0) {
        fprintf(stderr, "Nie można połączyć się z serwerem.\n");
        exit(EXIT_FAILURE);
    }

    printf("Połączono z serwerem %s:%d\n",
       inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    sleep(1); // Better UX
    printf("\033[2J\033[H");

    if(!authenticate(sockfd)){
        printf("Nie udało się uwierzytelnić. Zamykanie połączenia.\n");
        close(sockfd);
        return 1;
    }

    while (true) {
        int option = get_menu_option();
        if (option == OPTION_EXIT_GAME) {
            printf("Zamykanie połączenia.\n");
            break;
        }
        if (option == OPTION_PLAY) {
            game_client_init(sockfd);
        } else if (option == OPTION_VIEW_STATS) {
            get_statistics(sockfd);
        } else if (option == OPTION_ACTIVE_PLAYERS) {
            get_active_players(sockfd);
        }
    }


    close(sockfd);
    return 0;
}
