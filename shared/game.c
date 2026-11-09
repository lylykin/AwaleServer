#include "game.h"
#include <stdio.h>

/* Sème les graines dans le sens anti-horaire à partir de `trou_selected`.
 * Le plateau est une boucle de 12 trous : après le trou 5 d'une rangée,
 * on passe au trou 0 de l'autre rangée (c'est le `t == 6`). Le grenier
 * est sauté pendant le semis - les captures sont gérées par
 * mettre_dans_grenier(). Retourne le trou où la dernière graine est tombée. */
Position repart_seed(int seeds, int trou_selected, int joueur,
                     int plateau[NB_JOUEURS][NB_TROUS]) {
  int t = trou_selected;
  int j = joueur;

  while (seeds > 0) {
    // joueur 0 en haut, joueur 1 en bas.
    if (t == 6) {
      t = 0;
      j = (j + 1) % 2;
    }
    plateau[j][t] += 1;
    seeds--;
    t++;
  }
  Position fin = {j, t};
  return fin;
}

// simplification : on ne capture que les graines dans le trou sur lequel on a fini (si du coté de l'adversaire)
void mettre_dans_grenier(int joueur, Position fin, int plateau[NB_JOUEURS][NB_TROUS],
                         int grenier[2]) {
  int camp = fin.camp;
  int t = fin.trou - 1;

 if (plateau[camp][t] != 1 && camp != joueur) {
     grenier[joueur] += plateau[camp][t];
   plateau[camp][t] = 0;
 }
}

/* La partie se termine quand une rangée est vide. Ici le gagnant est
 * simplement celui qui a le plus de graines dans son grenier. */
int end_game(int plateau[NB_JOUEURS][NB_TROUS]) {
  int sum_0 = 0;
  int sum_1 = 0;

  // end condition : one of the player's board must be empty
  for (int i = 0; i < NB_TROUS; i++) {
    sum_0 += plateau[0][i];
    sum_1 += plateau[1][i];
  }

  return (sum_0 == 0 || sum_1 == 0);
}

int play_move(Game *game, int trou) {
      if (trou < 1 || trou > NB_TROUS) {
          return -1;
      }

      int seeds = game->plateau[game->joueur][trou - 1];
      if (seeds == 0) {
          return -2;
      }

      game->plateau[game->joueur][trou - 1] = 0;
      Position fin = repart_seed(seeds, trou, game->joueur, game->plateau);
      mettre_dans_grenier(game->joueur, fin, game->plateau, game->grenier);
      game->joueur = (game->joueur + 1) % 2;

      return 0;
  }

void init_game(Game *game, int premier) {
  for (int j = 0; j < NB_JOUEURS; j++) {
    for (int t = 0; t < NB_TROUS; t++) {
      game->plateau[j][t] = 4;
    }
    game->grenier[j] = 0;
  }
  game->joueur = premier;
}

void print_board(const Game *game, int viewer_side,
                 const char *name0, const char *name1) {
  int bottom = (viewer_side >= 0) ? viewer_side : game->joueur;
  int top = 1 - bottom;
  int turn = game->joueur;
  int last = 1 - turn;  /* le joueur qui vient de jouer */
  const char *names[2] = { name0, name1 };

  printf("\nJ%d a joue (%s)\n", last, names[last]);

  printf("Joueur %d : | ", top);
  for (int i = NB_TROUS - 1; i >= 0; i--) {
    printf("%2d | ", game->plateau[top][i]);
  }
  printf(" J%d = %d\n", top, game->grenier[top]);

  printf("------------------------------\n");

  printf("Joueur %d : | ", bottom);
  for (int i = 0; i < NB_TROUS; i++) {
    printf("%2d | ", game->plateau[bottom][i]);
  }
  printf(" J%d = %d\n", bottom, game->grenier[bottom]);

  if (viewer_side >= 0 && turn == viewer_side) {
    printf("C'est votre tour !\n");
  } else {
    printf("Tour du J%d (adversaire : %s)\n", turn, names[turn]);
  }
}
