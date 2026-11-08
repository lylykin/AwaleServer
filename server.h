#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#include "game.h"
#include "decodeur.h"

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100
#define MAX_MATCHES     (MAX_CLIENTS / 2)
#define MAX_FRIENDS     20
#define MAX_BIO_LINES   10
#define MAX_BIO_LENGTH  256
#define MAX_SAVED_GAMES 50

#define BUF_SIZE    4096

// Bio structure
typedef struct {
    char lines[MAX_BIO_LINES][MAX_BIO_LENGTH];
    int line_count;
} Bio;

// Saved game structure
typedef struct {
    char noms[2][MAX_NOM];
    Game g;
    int winner; // -1: draw, 0: player 0, 1: player 1
} SavedGame;

typedef struct{
    SOCKET sock;
    char name[MAX_NOM];
    int match; //-1:pas de partie en cours;>=0:indice de la partie
    int side;//0 ou 1 correspond à plateau[camp]
    char pending[MAX_NOM];//qui m'envoie un défi
    Bio bio;
    char friends[MAX_FRIENDS][MAX_NOM];
    int friend_count;
    int private_mode; // 0: public, 1: private
    int active; // 0: slot is free, 1: slot is in use
} Client;

typedef struct{
    int actif;//match en cours:1 matchfini:0
    Game g;
    char noms[2][MAX_NOM];//sauvgarder les noms des joueurs
    int side_of[2];//side (0 ou 1) de noms[0] et noms[1], pour la reconnexion
    Client *observers[MAX_CLIENTS];
    int observer_count;
    int saved; // 1 if game should be saved
}Match;

Match matches[MAX_MATCHES];
SavedGame saved_games[MAX_SAVED_GAMES];
int saved_game_count;

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
static void send_list(Client *clients, int actual, SOCKET to);
static int find_client(Client *clients, int actual, const char *name);
static void handle_message(Client *clients, int actual, int i, Message msg);
static void resume_match(Client *clients, int actual, int i);
static void start_match(Client *clients, int actual, int adv_idx, int me_idx);
static void handle_move(Client *clients, int actual, int i, int trou);
static void save_game(Match *m, int winner);
#endif /* guard */
