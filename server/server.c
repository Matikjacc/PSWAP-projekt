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

#include <signal.h>       // signal(), SIGHUP, SIG_IGN
#include <syslog.h>       // openlog(), syslog(), LOG_*
#include <sys/stat.h>     // umask()
#include <sys/types.h>    // pid_t, uid_t
#include <sys/resource.h> // getrlimit(), struct rlimit

#include "auth.h"
#include "protocol.h"
#include "ranking.h"
#include "lobby.h"
#include "game.h"
#include "storage.h"
#include "server_discovery.h"
#include "config.h"

#define MAXFD 64

int
daemon_init(const char *pname, int facility, uid_t uid)
{
	int		i, p;
	pid_t	pid;

	if ( (pid = fork()) < 0)
		return (-1);
	else if (pid)
		exit(0);			/* parent terminates */

	/* child 1 continues... */

	if (setsid() < 0)			/* become session leader */
		return (-1);

	signal(SIGHUP, SIG_IGN);
	if ( (pid = fork()) < 0)
		return (-1);
	else if (pid)
		exit(0);			/* child 1 terminates */

	/* child 2 continues... */

	chdir("/tmp");				/* change working directory  or chroot()*/
//	chroot("/tmp");

	/* close off file descriptors */
	for (i = 0; i < MAXFD; i++){
        close(i);
	}

	/* redirect stdin, stdout, and stderr to /dev/null */
	p= open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);

	openlog(pname, LOG_PID, facility);
	
        syslog(LOG_ERR," STDIN =   %i\n", p);
	setuid(uid); /* change user */
	
	return (0);				/* success */
}

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
        end_user_games(client_fd);
        remove_logged_user_by_fd(client_fd);
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
        if (authenticate_user(login, password, client_fd) == 0) {
            printf("Użytkownik \"%s\" zalogowany pomyślnie.\n", login);
        } else {
            printf("Błąd logowania dla użytkownika \"%s\".\n", login);
            msg.type = MSG_LOGIN_FAILURE;
            msg.length = 0;
            send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
        }
        return;
    } else if (msg.type == MSG_REGISTER) {
        const char* login = msg.value;
        const char* password = msg.value + strlen(login) + 1;
        if (register_user(login, password, client_fd) == 0) {
            printf("Użytkownik \"%s\" zarejestrowany pomyślnie.\n", login);
        } else {
            printf("Błąd rejestracji użytkownika \"%s\".\n", login);
            msg.type = MSG_REGISTER_FAILURE;
            msg.length = 0;
            send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
        }
        return;
    } 
    
    //Authenticated messages only
    AuthenicatedMessage auth_msg;
    memcpy(&auth_msg, msg.value, msg.length);
    if(find_user_by_id(auth_msg.player_id) < 0) {
        fprintf(stderr, "Błąd: Nie znaleziono użytkownika o ID %d.\n", auth_msg.player_id);
        msg.type = MSG_ERROR;
        msg.length = 0;
        send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
        return;
    }
    if(msg.type == MSG_RANKING) {
        printf("Otrzymano żądanie rankingu.\n");
        char* ranking = get_statistics();
        if (ranking == NULL) {
            fprintf(stderr, "Błąd podczas pobierania rankingu.\n");
            msg.type = MSG_ERROR;
            msg.length = 0;
            send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
            return;
        }

        size_t ranking_len = strlen(ranking) + 1; 
        strncpy(msg.value, ranking, MAX_PAYLOAD - 1);
        msg.value[MAX_PAYLOAD - 1] = '\0'; 
        
        msg.type = MSG_RANKING_RESPONSE;
        msg.length = ranking_len;

        ssize_t total_size = sizeof(msg.type) + sizeof(msg.length) + msg.length;
        if (send(client_fd, &msg, total_size, 0) < 0) {
            perror("send ranking");
            return;
        }

        printf("Wysłano odpowiedź z rankingiem:\n%s\n", ranking);
    } else if(msg.type == MSG_ACTIVE_USERS) {
        printf("Otrzymano żądanie aktywnych graczy.\n");
        char* active_users = get_active_users_list();
        if (active_users == NULL) {
            fprintf(stderr, "Błąd podczas pobierania listy aktywnych graczy.\n");
            msg.type = MSG_ERROR;
            msg.length = 0;
            send(client_fd, &msg, sizeof(msg.type) + sizeof(msg.length), 0);
            return;
        }

        size_t active_users_len = strlen(active_users) + 1; 
        strncpy(msg.value, active_users, MAX_PAYLOAD - 1);
        msg.value[MAX_PAYLOAD - 1] = '\0'; 
        
        msg.type = MSG_ACTIVE_USERS_RESPONSE;
        msg.length = active_users_len;

        ssize_t total_size = sizeof(msg.type) + sizeof(msg.length) + msg.length;
        if (send(client_fd, &msg, total_size, 0) < 0) {
            perror("send active users");
            return;
        }

        printf("Wysłano odpowiedź z aktywnymi graczami:\n%s\n", active_users);
    } else if(msg.type == MSG_JOIN_LOBBY) {
        int player_id;
        player_id = auth_msg.player_id;
        printf("Klient %d chce dołączyć do lobby.\n", player_id);
        lobby_join(client_fd, player_id);
    }else if(msg.type == MSG_MOVE){
        Move move;
        memcpy(&move, auth_msg.value, sizeof(Move));
        printf("Otrzymano ruch od gracza %d.\n", auth_msg.player_id);
        game_make_move(&move, auth_msg.player_id);
    }else {
        fprintf(stderr, "Nieznany typ wiadomości: %u\n", msg.type);
    }
}

int main() {

    // Inicjalizacja demona
    if (daemon_init("game_server", LOG_USER, 1000) < 0) {
        perror("daemon_init");
        exit(EXIT_FAILURE);
    }

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

    int discovery_fd = init_discovery_socket();
    if (discovery_fd >= 0) {
        ev.events = EPOLLIN;
        ev.data.fd = discovery_fd;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, discovery_fd, &ev) == -1) {
            perror("epoll_ctl discovery");
            close(discovery_fd);
        } else {
            printf("[Discovery] Nasłuchuję multicast na %s:%d\n", MULTICAST_GROUP, MULTICAST_PORT);
        }
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
            } else if (events[n].data.fd == discovery_fd) {
                // Obsługa multicast server discovery
                handle_discovery_message(discovery_fd);
            }
            else {
                // Obsługa danych od klienta
                handle_client_message(events[n].data.fd);
            }
        }
    }

    close(server_fd);
    close(epollfd);
    return 0;
}
