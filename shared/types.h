#ifndef TYPES_H
#define TYPES_H

/* types.h : types centraux partagés par tous les modules du serveur.
 * Les tableaux dynamiques (observateurs, parties sauvegardées, amis,
 * bio) utilisent la lib rxi/vec : plus de plafond artificiel ni de
 * memmove manuel lors des suppressions. */

#include "platform.h"
#include "vec.h"
#include "game.h"
#include "decodeur.h"

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100         /* soft cap for connected players + listen backlog */
#define MAX_FRIENDS     20          /* soft cap for friend list             */
#define MAX_BIO_LINES   10          /* soft cap for bio lines               */
#define MAX_BIO_LENGTH  256
#define BUF_SIZE    4096

/* vec_t(char[N]) n'est pas du C valide, d'où ces structs enveloppes */
typedef struct { char value[MAX_NOM]; } FriendName;
typedef struct { char value[MAX_BIO_LENGTH]; } BioLine;

struct Client;
typedef vec_t(struct Client*) ClientPtrVec; /* clients connectés + observateurs */
typedef vec_t(FriendName) FriendVec;        /* liste d'amis                     */
typedef vec_t(BioLine) BioLineVec;           /* lignes de la bio                 */

typedef struct {
    BioLineVec lines;
} Bio;

typedef struct {
    char noms[2][MAX_NOM];
    Game g;
    int winner; /* -1: nul, 0: joueur 0, 1: joueur 1 */
} SavedGame;

typedef vec_t(SavedGame) SavedGameVec;

/* ClientPlayer : identité persistante d'un joueur, partagée entre le
 * runtime et la couche persistance (JSON). Tous les champs ici sont
 * sérialisables ; les champs transient (socket, pending, active) vivent
 * dans Client. */
typedef struct {
    char name[MAX_NOM];
    Bio bio;
    FriendVec friends;
    int private_mode; /* 0: public, 1: privé */
    int match;        /* -1: pas en partie, >=0: index dans le vec matches */
    int side;       /* 0 ou 1 : côté du plateau joué par ce joueur  */
} ClientPlayer;

typedef vec_t(ClientPlayer*) PlayerPtrVec;  /* magasin persistant des identités */

typedef struct Client {
    SOCKET sock;
    int active;              /* 0: slot libre, 1: slot utilisé */
    char pending[MAX_NOM];   /* nom du joueur qui nous a défié */
    char pending_friend[MAX_NOM]; /* nom du joueur qui nous a demandé en ami */
    ClientPlayer *player;    /* identité persistante liée à ce slot */
} Client;

typedef struct {
    int actif;              /* 1: partie en cours ou en pause, 0: terminée */
    int paused;             /* 1: aucun des deux joueurs n'est connecté */
    Game g;
    char noms[2][MAX_NOM];
    int side_of[2];         /* côté de chaque joueur, pour la reconnexion */
    ClientPtrVec observers; /* observateurs connectés */
    int saved;
} Match;

typedef vec_t(Match) MatchVec;

#endif /* TYPES_H */
