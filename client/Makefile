CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I../common
OBJS = client.o network.o ui.o game.o ../common/utils.o

all: client

client: $(OBJS)
	$(CC) $(CFLAGS) -o client $(OBJS)

clean:
	rm -f *.o client
