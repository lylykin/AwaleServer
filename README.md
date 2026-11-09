# AwaleServer

Un serveur TCP qui permet à plusieurs joueurs
de se connecter, de discuter et de jouer à l'Awalé les uns contre les autres.

## Démarrage rapide

```bash
make                          # compile bin/server et bin/client
./bin/server                  # terminal 1 : lance le serveur (port 1977)
./bin/client 127.0.0.1 Alice  # terminal 2 : premier joueur
./bin/client 127.0.0.1 Bob    # terminal 3 : deuxième joueur
```

## Compilation

- `make`        : compile `bin/server` et `bin/client`       
- `make clean`  : supprime les fichiers compilés             

## Lancement

Syntaxe client : `./bin/client <adresse du serveur> <pseudo>`.
Pour jouer depuis une autre machine, remplacez `127.0.0.1` par l'adresse IP
de la machine qui exécute le serveur. Le pseudo est limité à 31 caractères.

Serveru: n'importe quelle touche arrête
le serveur.

### Quitter

`Ctrl+C` (ou `Ctrl+D`) dans le terminal d'un client pour le déconnecter.
Les autres joueurs sont prévenus de la déconnexion.

## Lecture du plateau

```
J0 a joue (Alice)
Joueur 1 : |  4 |  4 |  4 |  4 |  4 |  5 |  J1 = 0
------------------------------
Joueur 0 : |  4 |  4 |  0 |  5 |  5 |  5 |  J0 = 0
Tour du J1 (adversaire : Bob)
```

Chaque joueur voit sa propre rangée en bas avec son grenier à droite.
En haut : qui vient de jouer. En bas : `C'est votre tour !` ou le tour de
l'adversaire. Les graines tournent dans le sens inverse des aiguilles
d'une montre : la rangée du haut se lit de droite à gauche (trous 6 à 1).
