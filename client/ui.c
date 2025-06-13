#include "ui.h"
#include "network.h"
#include <stdio.h>
#include <protocol.h>
#include <string.h>
#include <unistd.h>


bool authenticate(int sockfd){
    char username[50];
    char password[50];

    printf("Witaj w systemie logowania!\n");
    printf("Wybierz opcję:\n");
    printf("%d. Zaloguj się\n", OPTION_LOGIN);
    printf("%d. Zarejestruj się\n", OPTION_REGISTER);
    printf("%d. Zapomniałem hasła\n", OPTION_FORGOT_PASSWORD);
    printf("%d. Wyjdź\n", OPTION_EXIT);

    int option;
    printf("Wybierz opcję (1-4): ");
    if (scanf("%d", &option) != 1 || option < OPTION_LOGIN || option > OPTION_EXIT) {
        fprintf(stderr, "Nieprawidłowa opcja.\n");
        return false;
    }
    getchar(); // Usuwamy znak nowej linii po scanf
    if (option == OPTION_EXIT) {
        printf("Dziękujemy za skorzystanie z systemu logowania!\n");
        return false;
    }
    if (option == OPTION_FORGOT_PASSWORD) {
        printf("Funkcja 'Zapomniałem hasła' nie jest jeszcze zaimplementowana.\n");
        return false;
    }
    if (option == OPTION_REGISTER) {
        printf("Podaj nazwę użytkownika: ");
        fgets(username, sizeof(username), stdin);
        printf("Podaj hasło: ");
        fgets(password, sizeof(password), stdin);

        username[strcspn(username, "\n")] = 0;
        password[strcspn(password, "\n")] = 0;
        if (strlen(username) == 0 || strlen(password) == 0) {
            fprintf(stderr, "Nazwa użytkownika i hasło nie mogą być puste.\n");
            return false;
        }

        register_account(sockfd, username, password);
        return true;
    }
    if( option == OPTION_LOGIN) {
        printf("Podaj nazwę użytkownika: ");
        fgets(username, sizeof(username), stdin);
        printf("Podaj hasło: ");
        fgets(password, sizeof(password), stdin);

        username[strcspn(username, "\n")] = 0;
        password[strcspn(password, "\n")] = 0;
        if (strlen(username) == 0 || strlen(password) == 0) {
            fprintf(stderr, "Nazwa użytkownika i hasło nie mogą być puste.\n");
            return false;
        }
        printf("Próba logowania...\n");
        return login(sockfd, username, password);
    }

    return false;
}

int get_menu_option() {
    int option;
    printf("\033[2J\033[H");
    printf("1. Graj\n");
    printf("2. Wyświetl statystyki\n");
    printf("3. Wyjdź\n");
    printf("Wybierz opcję (1-4): ");
    if (scanf("%d", &option) != 1 || option < OPTION_LOGIN || option > OPTION_EXIT) {
        fprintf(stderr, "Nieprawidłowa opcja.\n");
        return -1;
    }
    getchar();
    return option;
}

int get_statistics(int sockfd){
    TLVMessage msg;
    msg.type = MSG_RANKING;
    msg.length = 0;
    if (send(sockfd, &msg, sizeof(msg.type) + sizeof(msg.length), 0) < 0) {
        perror("send");
        return -1;
    }
    TLVMessage response;
    ssize_t bytes_received = recv(sockfd, &response, sizeof(response), 0);
    if (bytes_received < 0) {
        perror("recv");
        return -1;
    } else if (bytes_received == 0) {
        printf("Połączenie z serwerem zostało zamknięte.\n");
        return -1;
    }
    if (response.type == MSG_RANKING_RESPONSE) {
        printf("Ranking:\n%s\n", response.value);
        getchar();
    } else {
        fprintf(stderr, "Otrzymano nieprawidłową odpowiedź od serwera.\n");
        return -1;
    }
    return 0;
}