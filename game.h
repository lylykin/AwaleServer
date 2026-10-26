#include <stdio.h>
#define MAX_GRAINS 48
#define MAX_NOM 32
#define NB_TROUS 6

int plateau[2][6] = { {4,4,4,4,4,4}, {4,4,4,4,4,4}};
int grenier[2] = {0,0};

typedef struct Joueur{
    char nom[MAX_NOM];
}Joueur;
typedef struct{
    int camp;
    int trou;
}Position;


void winner(int* grenier);

int end_game(int (*plateau)[NB_TROUS]);

void show_plateau(int (*plateau)[NB_TROUS],  int* grenier);

Position repart_seed(int seeds, int trou_selected, int joueur, int (*plateau)[NB_TROUS]);


void mettre_dans_grenier(int joueur,Position fin, int plateau[2][6], int grenier[2]);

int game(int (*plateau)[NB_TROUS], int * grenier);
