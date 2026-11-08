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

void init_game(Game *g, int premier);
int play_move(Game *g, int trou);

int end_game(int plateau[NB_JOUEURS][NB_TROUS]);

Position repart_seed(int seeds, int trou_selected, int joueur,
                     int plateau[NB_JOUEURS][NB_TROUS]);

void mettre_dans_grenier(int joueur, Position fin,
                         int plateau[NB_JOUEURS][NB_TROUS], int grenier[2]);

void board_to_string(const Game *g, char *out, size_t taille);
