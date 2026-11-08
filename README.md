# AwaleServer

A small educative game server : un serveur TCP qui permet à plusieurs joueurs
de se connecter, de discuter et de jouer à l'Awalé les uns contre les autres.

## 1. Compilation

Le projet se compile sous Linux (ou WSL sous Windows). Dans le dossier du projet :

```bash
make
```

Cela produit deux exécutables : `server` et `client`.

| Commande      | Effet                                                        |
|---------------|--------------------------------------------------------------|
| `make`        | compile `server` et `client`                                 |
| `make clean`  | supprime les fichiers compilés                               |
| `make solo`   | compile l'ancienne version locale (`awale_game`, 2 joueurs sur le même terminal) |

## 2. Lancement

Ouvrez **trois terminaux** (sous Windows : trois terminaux, et `wsl` dans chacun),
placez-vous dans le dossier du projet, puis :

**Terminal 1 — le serveur**

```bash
./server
```

Le serveur n'affiche rien : c'est normal, il attend les connexions (port `1977`).
**Ne tapez rien dans ce terminal** : n'importe quelle touche arrête le serveur.

**Terminal 2 — premier joueur**

```bash
./client 127.0.0.1 Alice
```

**Terminal 3 — deuxième joueur**

```bash
./client 127.0.0.1 Bob
```

Syntaxe : `./client <adresse du serveur> <pseudo>`.
Pour jouer depuis une autre machine, remplacez `127.0.0.1` par l'adresse IP
de la machine qui exécute le serveur. Le pseudo est limité à 31 caractères.

## 3. Commandes disponibles

### Hors partie

| Commande            | Effet                                                       |
|---------------------|-------------------------------------------------------------|
| `/list`             | affiche la liste des joueurs connectés                      |
| `/challenge <pseudo>` | défie un joueur (ex. `/challenge Bob`)                    |
| `o`                 | accepter le défi reçu                                       |
| `n`                 | refuser le défi reçu                                        |
| *tout autre texte*  | message de chat envoyé à tous les joueurs connectés         |

Quand quelqu'un vous défie, le client affiche :

```
Alice vous défie ! Accepter ? (o/n)
```

### Pendant une partie

| Commande            | Effet                                                       |
|---------------------|-------------------------------------------------------------|
| `MOVE <n>`          | jouer le trou `n` (de 1 à 6), ex. `MOVE 3`                  |
| `<n>`               | raccourci : taper simplement `3`                            |

- `MOVE`, `Move` et `move` sont acceptés.
- C'est toujours **le joueur qui a lancé le défi (joueur 0) qui commence**.
- Après chaque coup, **les deux joueurs** reçoivent le nouveau plateau et
  le nom du joueur dont c'est le tour.
- Si ce n'est pas votre tour, le serveur répond `Ce n'est pas ton tour.`
- Si le trou est invalide (hors de 1–6 ou vide) : `Coup invalide.`
- À la fin de la partie, le serveur annonce le gagnant (`GAME_OVER ...`).

### Quitter

`Ctrl+C` (ou `Ctrl+D`) dans le terminal d'un client pour le déconnecter.
Les autres joueurs reçoivent `<pseudo> disconnected !`.

## 4. Exemple de partie

Chez **Alice** :

```
/challenge Bob
Défi envoyé à Bob.
ACCEPTED Bob
GAME_START Bob 0
YOUR_TURN

Joueur 0 : |  4 |  4 |  4 |  4 |  4 |  4 |
           ------------------------------
Joueur 1 : |  4 |  4 |  4 |  4 |  4 |  4 |
Grenier : J0 = 0, J1 = 0
Tour du joueur 0
Vous etes le joueur 0. Tapez MOVE <1-6> pour jouer.
MOVE 1
...
Alice a joue le trou 1. C'est au tour de Bob.
```

Chez **Bob** :

```
Alice vous défie ! Accepter ? (o/n)
o
GAME_START Alice 1
WAIT
...
Alice a joue le trou 1. C'est au tour de Bob.
6
...
Bob a joue le trou 6. C'est au tour de Alice.
```

## 5. Lecture du plateau

```
Joueur 0 : |  4 |  4 |  4 |  4 |  4 |  4 |      <- trous 6 5 4 3 2 1 du joueur 0
           ------------------------------
Joueur 1 : |  4 |  4 |  4 |  4 |  4 |  4 |      <- trous 1 2 3 4 5 6 du joueur 1
Grenier : J0 = 0, J1 = 0
```

Les graines tournent dans le sens inverse des aiguilles d'une montre :
la ligne du joueur 0 se lit donc de droite à gauche. Le `Grenier` indique
le nombre de graines capturées par chaque joueur.

## 6. Problèmes fréquents

| Message                               | Cause / solution                                         |
|---------------------------------------|----------------------------------------------------------|
| `connect(): Connection refused`       | le serveur n'est pas lancé (ou a été arrêté en tapant dans son terminal) |
| `bind(): Address already in use`      | un autre `server` tourne déjà : `pkill server` puis relancer |
| `undefined reference to 'init_game'`  | compilation sans `game.c` : utilisez `make`              |
| rien ne se passe sous PowerShell      | lancez les programmes depuis `wsl`                       |
