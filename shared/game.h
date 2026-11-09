#include <stdio.h>
#define MAX_NOM 32
#define NB_TROUS 6
#define NB_JOUEURS 2
#pragma once

typedef struct {
  int camp;
  int trou;
} Position;
typedef struct {
  int plateau[2][NB_TROUS];
  int grenier[2];
  int joueur;
} Game;

void init_game(Game *game, int premier);
int play_move(Game *game, int trou);

int end_game(int plateau[NB_JOUEURS][NB_TROUS]);

Position repart_seed(int seeds, int trou_selected, int joueur,
                     int plateau[NB_JOUEURS][NB_TROUS]);

void mettre_dans_grenier(int joueur, Position fin,
                         int plateau[NB_JOUEURS][NB_TROUS], int grenier[2]);

/* Affiche le plateau du point de vue du joueur local (viewer_side) :
 * sa rangée en bas, celle de l'adversaire en haut (inversée), grenier à
 * droite. En haut : qui a joué. En bas : de qui est le tour.
 * viewer_side = -1 : vue par défaut (game->joueur). */
void print_board(const Game *game, int viewer_side,
                 const char *name0, const char *name1);
