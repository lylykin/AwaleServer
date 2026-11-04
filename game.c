#include "game.h"
#include <stdio.h>
int plateau[2][6]= { {4,4,4,4,4,4}, {4,4,4,4,4,4}};
int grenier[2] = {0,0};
char* nom_joueur_1 = "Clelie";
char* nom_joueur_2 = "Deborah";


void winner(int* grenier, char* nom_joueur_1, char* nom_joueur_2) {

    printf("========================FIN DE LA PARTIE=============================");
    if (grenier[0] > grenier[1]) {
        printf( "Victoire de %s", nom_joueur_1);
    }
    else if (grenier[1] > grenier[0]) {
        printf( "Victoire de %s", nom_joueur_2);
    }
    else {
        printf("Egalité entre les deux joueurs");
    }
}

int end_game(int plateau[NB_JOUEURS][NB_TROUS]) {
  int sum_0 = 0;
  int sum_1 = 1;
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

void show_plateau(int plateau[NB_JOUEURS][NB_TROUS], int *grenier, int joueur, char* nom_joueur_1, char* nom_joueur_2) {

if (joueur == 1) {


  printf("\n %s : |  ", nom_joueur_1);
  for (int i = NB_TROUS - 1; i >= 0; i--) {
    printf("%d  |  ", plateau[0][i]);
  }
  printf("\n             ---------------------------------- \n");
  printf(" %s : |  ", nom_joueur_2);
  for (int i = 0; i < NB_TROUS; i++) {
    printf("%d  |  ", plateau[1][i]);
  }
  printf(" \n \n Grenier : \n %s = %d \n %s = %d\n\n",nom_joueur_1, grenier[0], nom_joueur_2, grenier[1]);
}
else {
    printf("\n %s : |  ", nom_joueur_2);
     for (int i = NB_TROUS - 1; i >= 0; i--) {
       printf("%d  |  ", plateau[1][i]);
     }
     printf("\n             ---------------------------------- \n");
     printf(" %s : |  ", nom_joueur_1);
     for (int i = 0; i < NB_TROUS; i++) {
       printf("%d  |  ", plateau[0][i]);
     }
     printf(" \n \n Grenier : \n %s = %d \n %s = %d\n\n",nom_joueur_1, grenier[0], nom_joueur_2, grenier[1]);
   }
}

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
    // if (j == joueur && t == trou_selected)
      // continue;
    plateau[j][t] += 1;
    seeds--;
    t++;
  }
  Position fin = {j, t};
  return fin;
}

void mettre_dans_grenier(int joueur, Position fin, int plateau[NB_JOUEURS][NB_TROUS],
                         int grenier[2]) {
  int camp = fin.camp;
  int t = fin.trou - 1;
  int capture = 0;
  printf("Valeurs : \n camp : %d \n t : %d\n", camp, t);

  if (plateau[camp][t] != 1 && camp != joueur) {
    capture += plateau[camp][t];
    plateau[camp][t] = 0;
    t--;
  }
  grenier[joueur] += capture;
}

int game(int plateau[NB_JOUEURS][NB_TROUS], int *grenier) {
  // int somme_grains = 0;
  int joueur = 0;
  int trou_selected;
  int seeds;
  Position trou_fin;

  // condition d'arret : un joueur ne peut plus joueur
  while (!end_game(plateau)) {

    // démarrage du tour du joueur
    show_plateau(plateau, grenier, joueur, nom_joueur_1, nom_joueur_2);
    printf("Joueur %d : Sélectionnez un trou : ", joueur + 1);
    scanf("%d", &trou_selected);

    seeds = plateau[joueur][trou_selected - 1];

    while (trou_selected < 1 || trou_selected > 6 || seeds == 0) {
      fprintf(stderr, "Trou invalide. Merci de rentrer une bonne valeur\n");
      scanf("%d", &trou_selected);
      seeds = plateau[joueur][trou_selected - 1];
    }

    plateau[joueur][trou_selected-1] = 0;
    trou_fin = repart_seed(seeds, trou_selected, joueur, plateau);
    mettre_dans_grenier(joueur, trou_fin, plateau, grenier);
    joueur = (joueur + 1) % 2;

  }

  winner(grenier, nom_joueur_1, nom_joueur_2);
  return 0;
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
