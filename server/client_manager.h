#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

/* client_manager.h : gestion du vec de clients connectés (recherche,
 * déconnexion, broadcast). Les Client sont alloués sur le tas et stockés
 * par pointeur dans un ClientPtrVec - les pointeurs restent stables même
 * quand le vec réalloue. */

#include "../shared/types.h"

/* players : magasin persistant des identités ClientPlayer (allouées sur le
 * tas, stockées par pointeur pour stabilité), partagé entre le runtime
 * (via Client->player) et la couche persistence (JSON). */
extern PlayerPtrVec players;

/* Cherche un client actif par nom. Retourne le Client* ou NULL. */
Client *find_client(ClientPtrVec *clients, const char *name);

/* Déconnecte un client : sauvegarde, nettoie observateurs et parties,
 * puis retire le Client du vec et le libère. */
void remove_client(ClientPtrVec *clients, Client *to_remove);

void clear_clients(ClientPtrVec *clients);

/* Envoie un message chat à tous les clients actifs sauf l'expéditeur.
 * from_server=1 : pas de préfixe "pseudo : ". */
void send_message_to_all_clients(ClientPtrVec *clients, Client *sender,
                                 const char *buffer, char from_server);

/* Envoie la liste des joueurs en ligne dans un champ structuré. */
void send_list(ClientPtrVec *clients, SOCKET to);

#endif /* CLIENT_MANAGER_H */
