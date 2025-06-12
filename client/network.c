#include <stdio.h>
#include "protocol.h"
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


void login(int sockfd, const char* login, const char* password) {
    TLVMessage msg;
    msg.type = MSG_LOGIN;

    size_t login_len = strlen(login);
    size_t password_len = strlen(password);
    msg.length = login_len + 1 + password_len + 1;

    if (msg.length > MAX_PAYLOAD) {
        fprintf(stderr, "Dane logowania za długie!\n");
        return;
    }

    // Kopiujemy login i hasło do bufora
    memset(msg.value, 0, sizeof(msg.value));
    memcpy(msg.value, login, login_len);
    memcpy(msg.value + login_len + 1, password, password_len);

    // Wysyłamy całą strukturę TLVMessage
    ssize_t total_size = sizeof(msg.type) + sizeof(msg.length) + msg.length;
    ssize_t sent = send(sockfd, &msg, total_size, 0);
    
    if (sent != total_size) {
        perror("send");
    } else {
        printf("Wysłano dane logowania: login=\"%s\", hasło=\"%s\"\n", login, password);
    }

    // Odbieramy odpowiedź od serwera
    TLVMessage response;
    ssize_t header_bytes = read(sockfd, &response, sizeof(response.type) + sizeof(response.length));
    if (header_bytes < 0) {
        perror("read");
        return;
    } else if (header_bytes == 0) {
        printf("Serwer zamknął połączenie.\n");
        return;
    }
    if (response.length > MAX_PAYLOAD) {
        fprintf(stderr, "Odpowiedź serwera jest zbyt długa (%u bajtów)\n", response.length);
        return;
    }
    ssize_t value_bytes = read(sockfd, response.value, response.length);
    if (value_bytes != response.length) {
        fprintf(stderr, "Nieprawidłowa liczba bajtów od serwera\n");
        return;
    }
    if (response.type == MSG_LOGIN_SUCCESS) {
        printf("Zalogowano pomyślnie!\n");
    } else if (response.type == MSG_LOGIN_FAILURE) {
        printf("Błąd logowania. Sprawdź login i hasło.\n");
    } else {
        printf("Otrzymano nieznany typ wiadomości: %u\n", response.type);
    }
}

void register_account(int sockfd, const char* login, const char* password){
    TLVMessage msg;
    msg.type = MSG_REGISTER;

    size_t login_len = strlen(login);
    size_t password_len = strlen(password);
    msg.length = login_len + 1 + password_len + 1;

    if (msg.length > MAX_PAYLOAD) {
        fprintf(stderr, "Dane rejestracji za długie!\n");
        return;
    }

    // Kopiujemy login i hasło do bufora
    memset(msg.value, 0, sizeof(msg.value));
    memcpy(msg.value, login, login_len);
    memcpy(msg.value + login_len + 1, password, password_len);

    // Wysyłamy całą strukturę TLVMessage
    ssize_t total_size = sizeof(msg.type) + sizeof(msg.length) + msg.length;
    ssize_t sent = send(sockfd, &msg, total_size, 0);
    
    if (sent != total_size) {
        perror("send");
    } else {
        printf("Wysłano dane rejestracji: login=\"%s\", hasło=\"%s\"\n", login, password);
    }

    // Odbieramy odpowiedź od serwera
    TLVMessage response;
    ssize_t header_bytes = read(sockfd, &response, sizeof(response.type) + sizeof(response.length));
    if (header_bytes < 0) {
        perror("read");
        return;
    } else if (header_bytes == 0) {
        printf("Serwer zamknął połączenie.\n");
        return;
    }
    if (response.length > MAX_PAYLOAD) {
        fprintf(stderr, "Odpowiedź serwera jest zbyt długa (%u bajtów)\n", response.length);
        return;
    }
    ssize_t value_bytes = read(sockfd, response.value, response.length);
    if (value_bytes != response.length) {
        fprintf(stderr, "Nieprawidłowa liczba bajtów od serwera\n");
        return;
    }
    if (response.type == MSG_REGISTER_SUCCESS) {
        printf("Rejestracja zakończona sukcesem!\n");
    } else if (response.type == MSG_REGISTER_FAILURE) {
        printf("Błąd rejestracji. Sprawdź login i hasło.\n");
    } else {
        printf("Otrzymano nieznany typ wiadomości: %u\n", response.type
        );
    }
}