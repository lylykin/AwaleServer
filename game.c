#include "game.h"
int plateau[2][6] = {{4,4,4,4,4,4},{4,4,4,4,4,4}};
int grenier[2] = {0,0};
void winner(int* grenier) {
    if (grenier[0] > grenier[1]) {
        printf( "Victoire du joueur 1");
    }
    else if (grenier[1] > grenier[0]) {
        printf( "Victoire du joueur 2");
    }
    else {
        printf("Egalité entre les deux joueurs");
    }
}

int end_game(int (*plateau)[NB_TROUS]) {
    int sum_0 = 0;
    int sum_1 = 1;
    int end_game = -1;

    for (int i = 0; i < NB_TROUS; i++) {
        sum_0 += plateau[0][i];
        sum_1 += plateau[1][i];
    }

    if (sum_0 == 0) {
        end_game = 0;
    }

    else if (sum_1 == 0) {
        end_game = 1;
    }

    return (end_game);
}

void show_plateau(int (*plateau)[NB_TROUS],  int* grenier) {
    printf ("\n Joueur 0 : |  ");
    for (int i = NB_TROUS - 1; i >=0; i--){
        printf("%d  |  ",plateau[0][i]);
    }
    printf("\n             ---------------------------------- \n");
    printf(" Joueur 1 : |  ");
    for (int i = 0; i < NB_TROUS; i++){
        printf("%d  |  ",plateau[1][i]);
    }
    printf(" \n \n Grenier : \n Joueur 0 = %d \n Joueur 1 = %d\n\n", grenier[0], grenier[1]);
}


Position repart_seed(int seeds, int trou_selected, int joueur, int (*plateau)[NB_TROUS]) {
    int t = trou_selected;
    int j = joueur;
    while (seeds > 0) {
        // joueur 0 en haut, joueur 1 en bas.
            if (t == 6) {
                t = 0;
                j = (j + 1) % 2;
            }
            if (j == joueur && t == trou_selected - 1) {
                t++;
                continue;
            }
            plateau[j][t] +=1;
            seeds--;
            t++;
    }
    Position fin = {j,t-1};
    return fin;
}

void mettre_dans_grenier(int joueur,Position fin, int plateau[2][6], int grenier[2]){
    int camp = fin.camp;
    int t = fin.trou;
    int capture = 0;

    if (camp != joueur) {
      while (t >= 0 && (plateau[camp][t] == 2 || plateau[camp][t] == 3)) {
          capture += plateau[camp][t];
          plateau[camp][t] = 0;
          t--;
      }
  }
    grenier[joueur]+=capture;
}

int game(void) {
      Game g;
      init_game(&g, 0);

      while (end_game(g.plateau) == -1) {
          show_plateau(g.plateau, g.grenier);
          printf("Joueur %d Sélectionnez un trou : ", g.joueur + 1);

          int trou;
          if (scanf("%d", &trou) != 1) {
              return 1;
          }

          int r = play_move(&g, trou);
          if (r == -1) {
              printf("Merci de rentrer une valeur entre 1 et 6\n");
          } else if (r == -2) {
              printf("Trou invalide : ce trou est vide\n");
          }
      }

      show_plateau(g.plateau, g.grenier);
      winner(g.grenier);
      return 0;
  }
void init_game(Game *g, int premier){
    for (int c = 0; c < 2; c++){
        for (int t = 0; t<NB_TROUS; t++){
           g->plateau[c][t] = 4;
        }
        g->grenier[c] = 0;
    }
    g->joueur = premier;
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