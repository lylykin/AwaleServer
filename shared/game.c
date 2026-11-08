#include "game.h"
#include <stdio.h>

/* Sème les graines dans le sens anti-horaire à partir de `trou_selected`.
 * Le plateau est une boucle de 12 trous : après le trou 5 d'une rangée,
 * on passe au trou 0 de l'autre rangée (c'est le `t == 6`). Le grenier
 * est sauté pendant le semis — les captures sont gérées par
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

/* Règle de capture (Awale simplifié) : si la dernière graine atterrit
 * chez l'adversaire (camp != joueur) et que ce trou contient 2 ou 3
 * graines, le joueur les capture dans son grenier. fin.trou - 1 reconvertit
 * le "trou suivant" en trou d'atterrissage réel. Ici on ne capture qu'un
 * seul trou (pas de capture en chaîne) — simplification volontaire. */
void mettre_dans_grenier(int joueur, Position fin, int plateau[NB_JOUEURS][NB_TROUS],
                         int grenier[2]) {
  int camp = fin.camp;
  int t = fin.trou - 1;
  int capture = 0;

  if (plateau[camp][t] != 1 && camp != joueur) {
    capture += plateau[camp][t];
    plateau[camp][t] = 0;
    t--;
  }
  grenier[joueur] += capture;
}

/* La partie se termine quand une rangée est vide. Ici le gagnant est
 * simplement celui qui a le plus de graines dans son grenier. */
int end_game(int plateau[NB_JOUEURS][NB_TROUS]) {
  int sum_0 = 0;
  int sum_1 = 0;
  int end_game = 0;

  // end condition : one of the player's board must be empty
  for (int i = 0; i < NB_TROUS; i++) {
    sum_0 += plateau[0][i];
    sum_1 += plateau[1][i];
  }

  if (sum_0 == 0 || sum_1 == 0) {
    end_game = 1;
  }

  return (end_game);
}

int play_move(Game *g, int trou) {
      if (trou < 1 || trou > NB_TROUS) {
          return -1;
      }

      int seeds = g->plateau[g->joueur][trou - 1];
      if (seeds == 0) {
          return -2;
      }

      g->plateau[g->joueur][trou - 1] = 0;
      Position fin = repart_seed(seeds, trou, g->joueur, g->plateau);
      mettre_dans_grenier(g->joueur, fin, g->plateau, g->grenier);
      g->joueur = (g->joueur + 1) % 2;

      return 0;
  }

void init_game(Game *g, int premier) {
  for (int j = 0; j < NB_JOUEURS; j++) {
    for (int t = 0; t < NB_TROUS; t++) {
      g->plateau[j][t] = 4;
    }
    g->grenier[j] = 0;
  }
  g->joueur = premier;
}

/* Version "réseau" de show_plateau : écrit le plateau dans `out`
 * au lieu de l'afficher. Utilisée par le serveur pour l'envoyer aux joueurs. */
void board_to_string(const Game *g, char *out, size_t taille) {
  size_t pos = 0;

  pos += snprintf(out + pos, taille - pos, "\nJoueur 0 : | ");
  for (int i = NB_TROUS - 1; i >= 0 && pos < taille; i--) {
    pos += snprintf(out + pos, taille - pos, "%2d | ", g->plateau[0][i]);
  }
  if (pos < taille)
    pos += snprintf(out + pos, taille - pos,
                    "\n           ------------------------------\nJoueur 1 : | ");
  for (int i = 0; i < NB_TROUS && pos < taille; i++) {
    pos += snprintf(out + pos, taille - pos, "%2d | ", g->plateau[1][i]);
  }
  if (pos < taille)
    snprintf(out + pos, taille - pos,
             "\nGrenier : J0 = %d, J1 = %d\nTour du joueur %d\r\n",
             g->grenier[0], g->grenier[1], g->joueur);
}
