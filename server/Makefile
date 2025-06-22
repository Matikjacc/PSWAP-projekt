CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I../common
OBJS = server.o auth.o lobby.o game.o ranking.o storage.o server_discovery.o ../common/utils.o

all: server

server: $(OBJS)
	$(CC) $(CFLAGS) -o server $(OBJS)

clean:
	rm -f *.o server
