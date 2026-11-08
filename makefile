CC      = gcc
# gnu11 (et pas c11) : server.h/client.h testent la macro `linux`,
# qui n'est pas définie en mode C strict
CFLAGS  = -Wall -Wextra -std=gnu11

# version réseau (cible par défaut)
all: server client

server: server.o game.o decodeur.o
	$(CC) -o $@ $^

client: client.o decodeur.o
	$(CC) -o $@ $^

server.o: server.c server.h game.h decodeur.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c client.h decodeur.h
	$(CC) $(CFLAGS) -c client.c

game.o: game.c game.h
	$(CC) $(CFLAGS) -c game.c

decodeur.o: decodeur.c decodeur.h
	$(CC) $(CFLAGS) -c decodeur.c

clean:
	rm -f *.o server client

.PHONY: all clean
