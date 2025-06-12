#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

#include "auth.h"
#include "protocol.h"

#define PORT 1234
#define BACKLOG 10

void handle_client(int client_fd) {
    TLVMessage msg;

    while (1) {
        ssize_t header_bytes = read(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length));
        if (header_bytes == 0) {
            printf("Klient się rozłączył.\n");
            break;
        } else if (header_bytes < 0) {
            perror("Błąd odczytu nagłówka");
            break;
        }

        if (msg.length > MAX_PAYLOAD) {
            fprintf(stderr, "Zbyt długi payload (%u bajtów)\n", msg.length);
            break;
        }

        ssize_t value_bytes = read(client_fd, msg.value, msg.length);
        if (value_bytes != msg.length) {
            fprintf(stderr, "Nieprawidłowa liczba bajtów od klienta\n");
            break;
        }

        printf("Odebrano wiadomość: typ=%u, długość=%u\n", msg.type, msg.length);
        
        if(msg.type == MSG_LOGIN) {
            const char* login = msg.value;
            const char* password = msg.value + strlen(login) + 1;
            printf("Login: \"%s\", Hasło: \"%s\"\n", login, password);
            if (authenticate_user(login, password) == 0) {
                printf("Użytkownik \"%s\" zalogowany pomyślnie.\n", login);
                // Można wysłać potwierdzenie do klienta
                msg.type = MSG_LOGIN_SUCCESS;
                msg.length = 0; // brak dodatkowych danych
                send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
            } else {
                printf("Błąd logowania dla użytkownika \"%s\".\n", login);
                msg.type = MSG_LOGIN_FAILURE;
                msg.length = 0; // brak dodatkowych danych
                send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
            }
        } else if (msg.type == MSG_REGISTER) {
            const char* login = msg.value;
            const char* password = msg.value + strlen(login) + 1;
            if (register_user(login, password) == 0) {
                printf("Użytkownik \"%s\" zarejestrowany pomyślnie.\n", login);
                msg.type = MSG_REGISTER_SUCCESS;
                msg.length = 0; // brak dodatkowych danych
                send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
            } else {
                printf("Błąd rejestracji użytkownika \"%s\".\n", login);
                msg.type = MSG_REGISTER_FAILURE;
                msg.length = 0; // brak dodatkowych danych
                send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
            }
        } else {
            fprintf(stderr, "Nieznany typ wiadomości: %u\n", msg.type);
        }
    }

    close(client_fd);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

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

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);        // port 1234
    server_addr.sin_addr.s_addr = INADDR_ANY;  // IPv4, dowolny interfejs

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Serwer nasłuchuje na porcie %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Połączono z klientem: %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        handle_client(client_fd);
    }

    close(server_fd);
    return 0;
}
