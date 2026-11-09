#ifndef PERSISTENCE_H
#define PERSISTENCE_H

/* persistence.h : sérialisation JSON des types runtime partagés (types.h).
 * Plus de types Persistent* séparés : on persiste directement ClientPlayer[]
 * et Match[] - les champs transient (sock, active, pending, observers) sont
 * simplement ignorés lors de la sérialisation. */

#include "../shared/types.h"
#include "../shared/cJSON.h"

#define DATA_FILE "datas.json"

/* Sauvegarde tous les joueurs (name non vide) et tous les matchs actifs. */
void save_persistence_data(const PlayerPtrVec *players, const MatchVec *matches);

/* Charge les joueurs dans le vec players (alloué sur le tas, poussé dans
 * le vec) et les matchs dans le vec matches. */
void load_persistence_data(PlayerPtrVec *players, MatchVec *matches);

/* Cherche un joueur par nom dans le vec persistant. Retourne le ClientPlayer*
 * ou NULL si introuvable. */
ClientPlayer *find_persistent_player(const PlayerPtrVec *players, const char *name);

#endif /* PERSISTENCE_H */
