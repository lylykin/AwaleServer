CC      = gcc
# gnu11 (et pas c11) : server.h/client.h testent la macro `linux`,
# qui n'est pas définie en mode C strict
CFLAGS  = -Wall -Wextra -std=gnu11 -Ishared -Iserver -Iclient

SHARED_SRC = shared/decodeur.c shared/game.c shared/vec.c shared/cJSON.c
SHARED_OBJ = $(SHARED_SRC:.c=.o)

SERVER_SRC = server/server.c server/network.c server/client_manager.c \
             server/match_manager.c server/handlers.c server/persistence.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)

CLIENT_SRC = client/client.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

all: bin/server bin/client

bin/server: $(SERVER_OBJ) $(SHARED_OBJ)
	@mkdir -p bin
	$(CC) -o $@ $^

bin/client: $(CLIENT_OBJ) shared/decodeur.o shared/game.o
	@mkdir -p bin
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SHARED_OBJ)
	rm -rf bin

.PHONY: all clean
