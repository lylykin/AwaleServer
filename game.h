#include <stdio.h>
#define MAX_GRAINS 48
#define MAX_NOM 32
#define NB_TROUS 6
#define NB_JOUEURS 2
#pragma once

// note : for scalability and security, should be updated as a static + getters
// architecture
extern int plateau[2][6];
extern int grenier[2];

typedef struct Joueur {
  char nom[MAX_NOM];
} Joueur;
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
void winner(int *grenier);

int end_game(int plateau[NB_JOUEURS][NB_TROUS]);

void show_plateau(int plateau[NB_JOUEURS][NB_TROUS], int *grenier, int joueur,
                  char *nom_joueur_1, char *nom_joueur_2);

Position repart_seed(int seeds, int trou_selected, int joueur,
                     int plateau[NB_JOUEURS][NB_TROUS]);

void mettre_dans_grenier(int joueur, Position fin,
                         int plateau[NB_JOUEURS][NB_TROUS], int grenier[2]);

int game(int plateau[NB_JOUEURS][NB_TROUS], int *grenier);
