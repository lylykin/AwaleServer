CC      = gcc
# gnu11 (et pas c11) : server.h/client.h testent la macro `linux`,
# qui n'est pas définie en mode C strict
CFLAGS  = -Wall -Wextra -std=gnu11

# version réseau (cible par défaut)
all: server client

server: server.o game.o
	$(CC) -o $@ $^

client: client.o
	$(CC) -o $@ $^

server.o: server.c server.h game.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c client.h
	$(CC) $(CFLAGS) -c client.c

game.o: game.c game.h
	$(CC) $(CFLAGS) -c game.c

# ancienne version locale (2 joueurs sur le même terminal) : make solo
solo: main.o game.o
	$(CC) -o awale_game $^

main.o: main.c game.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f *.o server client awale_game

.PHONY: all solo clean
