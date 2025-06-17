#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>

#include "client.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234


int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Połączono z serwerem %s:%d\n", SERVER_IP, SERVER_PORT);

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
            printf("Gra rozpoczęta!\n");
        } else if (option == OPTION_VIEW_STATS) {
            get_statistics(sockfd);
        }
    }


    close(sockfd);
    return 0;
}
