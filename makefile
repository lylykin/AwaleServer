CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11
LDFLAGS =

OBJS = main.o game.o
TARGET = awale_game

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

main.o: main.c game.h
	$(CC) $(CFLAGS) -c main.c

game.o: game.c game.h
	$(CC) $(CFLAGS) -c game.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
