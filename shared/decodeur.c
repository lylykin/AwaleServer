#include "decodeur.h"
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// Send a message to the server
int send_server(Message msg, int socket) {
    if (send(socket, &msg, sizeof(msg), 0) < 0) {
        perror("send()");
        return -1;
    }
    return 0;
}

// Send a message to a client
int send_client(Message msg, int sock) {
    if (send(sock, &msg, sizeof(msg), 0) < 0) {
        perror("send()");
        return -1;
    }
    return 0;
}

// Receive a message from server
Message msg_from_server(int socket) {
    Message msg;
    int n = recv(socket, &msg, sizeof(msg), 0);
    if (n < 0) {
        perror("recv()");
        msg.type = -1; // error
    } else if (n == 0) {
        msg.type = -1; // connection closed
    }
    return msg;
}

// Side (0 or 1) of the local player, so the board can be shown with
// the player's own row at the bottom. -1 = not in a game (default view).
static int player_side = -1;

void set_player_side(int side) {
    player_side = side;
}

// Client-side: handle a message from server.
// content[] is only used for MSG_CHAT; every other message type reads
// its dedicated union field.
void handle_msg_server(int sock, Message msg) {
    (void)sock;

    switch (msg.type) {
        case MSG_CHAT:
            printf("%s\n", msg.content);
            break;

        case MSG_LIST_RESPONSE:
            printf("Joueurs en ligne (%d) :", msg.player_list.count);
            for (int i = 0; i < msg.player_list.count; i++) {
                printf(" %s", msg.player_list.names[i]);
            }
            printf("\n");
            break;

        case MSG_SEND_CHALLENGE:
            printf("%s vous defie ! Accepter ? (o/n)\n",
                   msg.challenge.challenger_name);
            break;

        case MSG_ANSWER_CHALLENGE:
            if (msg.challenge.accepted) {
                printf("%s a accepte votre defi !\n", msg.challenge.challenged_name);
            } else {
                printf("%s a refuse votre defi.\n", msg.challenge.challenged_name);
            }
            break;

        case MSG_GAME_START:
            printf("Partie demarree contre %s. Vous etes le joueur %d.\n",
                   msg.challenge.challenged_name, msg.challenge.side);
            break;

        case MSG_BOARD_UPDATE: {
            int top = (player_side == 0) ? 1 : 0;
            int bottom = 1 - top;
            printf("Plateau :\n");
            printf("  %2d %2d %2d %2d %2d %2d | %2d\n",
                   msg.board.board[top][0], msg.board.board[top][1], msg.board.board[top][2],
                   msg.board.board[top][3], msg.board.board[top][4], msg.board.board[top][5],
                   msg.board.grenier[top]);
            printf("  %2d %2d %2d %2d %2d %2d | %2d\n",
                   msg.board.board[bottom][0], msg.board.board[bottom][1], msg.board.board[bottom][2],
                   msg.board.board[bottom][3], msg.board.board[bottom][4], msg.board.board[bottom][5],
                   msg.board.grenier[bottom]);
            printf("Tour du joueur : %d\n", msg.board.current_player);
            break;
        }

        case MSG_YOUR_TURN:
            printf("A vous de jouer. Entrez un numero de trou (1-6) :\n");
            break;

        case MSG_WAIT:
            printf("Attendez le coup de l'adversaire.\n");
            break;

        case MSG_GAME_OVER: {
            if (msg.game_over.winner == -1) {
                printf("Match nul (%d - %d)\n",
                       msg.game_over.score0, msg.game_over.score1);
            } else {
                printf("Gagnant : joueur %d (%d - %d)\n",
                       msg.game_over.winner,
                       msg.game_over.score0, msg.game_over.score1);
            }
            break;
        }

        case MSG_MOVE_INVALID:
            printf("Coup invalide.\n");
            break;

        case MSG_NOT_YOUR_TURN:
            printf("Ce n'est pas votre tour.\n");
            break;

        case MSG_LIST_GAMES_RESPONSE:
            printf("Parties en cours (%d) :\n", msg.game_list.count);
            for (int i = 0; i < msg.game_list.count; i++) {
                printf("  [%d] %s vs %s\n", i,
                       msg.game_list.games[i].player1,
                       msg.game_list.games[i].player2);
            }
            break;

        case MSG_GET_BIO_RESPONSE:
            printf("Bio du joueur :\n");
            for (int i = 0; i < msg.bio.line_count; i++) {
                printf("  %s\n", msg.bio.bio_lines[i]);
            }
            break;

        case MSG_SAVED_GAMES_RESPONSE:
            printf("Parties sauvegardees (%d) :\n", msg.saved_games_list.count);
            for (int i = 0; i < msg.saved_games_list.count; i++) {
                SavedGameInfo *g = &msg.saved_games_list.games[i];
                const char *result = (g->winner == -1) ? "Match nul"
                                   : (g->winner == 0) ? g->noms[0] : g->noms[1];
                printf("  %s vs %s - %s\n", g->noms[0], g->noms[1], result);
            }
            break;

        default:
            printf("Unknown message received (type: %d).\n", msg.type);
            break;
    }
}
