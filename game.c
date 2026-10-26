#include "game.h"

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
            if (j == joueur && t == trou_selected) continue;
            plateau[j][t] +=1;
            seeds--;
            t++;
    }
    Position fin = {j,t};
    return fin;
}

void mettre_dans_grenier(int joueur,Position fin, int plateau[2][6], int grenier[2]){
    int camp = fin.camp;
    int t = fin.trou;
    int capture = 0;

    if (plateau[camp][t] == 1) {
        capture += plateau[camp][t];
               plateau[camp][t] = 0;
               t--;
    }
    grenier[joueur]+=capture;
}

int game(int (*plateau)[NB_TROUS], int * grenier) {
    int somme_grains = 0;
    int joueur = 0;
    int trou_selected;
    int seeds;
    Position trou_fin;

    //condition d'arret : un joueur ne peux plus joueur
    while (end_game(plateau) == -1) {

        // démarrage du tour du joueur
        show_plateau(plateau, grenier);
        printf("Joueur %d Sélectionnez un trou : ", joueur + 1);
        scanf("%d", &trou_selected);

        while (trou_selected < 1 || trou_selected > 6){
          printf("Merci de rentrer une bonne valeur");
           scanf("%d", &trou_selected);

       }
       seeds = plateau[joueur][trou_selected-1];

       while (seeds == 0){
           printf("Trou invalide");
           scanf("%d", &trou_selected);
       }
        trou_fin = repart_seed(seeds,trou_selected,joueur,plateau);
        mettre_dans_grenier(joueur, trou_fin, plateau, grenier);
        joueur = (joueur + 1) %2;

    }

    winner(grenier);

    return 0;
}
