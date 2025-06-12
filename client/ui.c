#include "ui.h"
#include "network.h"
#include <stdio.h>
#include <protocol.h>
#include <string.h>
#include <unistd.h>

typedef enum{
    OPTION_LOGIN = 1,
    OPTION_REGISTER,
    OPTION_FORGOT_PASSWORD,
    OPTION_EXIT
} MenuOption;

bool authenticate(int sockfd){
    char username[50];
    char password[50];

    printf("Witaj w systemie logowania!\n");
    printf("Wybierz opcję:\n");
    printf("1. Zaloguj się\n");
    printf("2. Zarejestruj się\n");
    printf("3. Zapomniałem hasła\n");
    printf("4. Wyjdź\n");

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

        login(sockfd, username, password);
        return true;
    }

    return false;
}