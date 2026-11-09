#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client_manager.h"
#include "match_manager.h"
#include "persistence.h"

PlayerPtrVec players;

Client *find_client(ClientPtrVec *clients, const char *name)
// chercher un client dans la liste des données persistées
{
    for (int i = 0; i < clients->length; i++) {
        Client *c = clients->data[i];
        if (c->active && c->player && strcmp(c->player->name, name) == 0) {
            return c;
        }
    }
    return NULL;
}

void remove_client(ClientPtrVec *clients, Client *to_remove)
{
   if (!to_remove->active) return;

   ClientPlayer *player = to_remove->player;
   int was_in_match = player ? player->match : -1;
   char disconnected_name[MAX_NOM];
   if (player) {
       strncpy(disconnected_name, player->name, MAX_NOM - 1);
       disconnected_name[MAX_NOM - 1] = '\0';
   } else {
       disconnected_name[0] = '\0';
   }

   /* Sauvegarde l'état (si partie en cours). */
   save_persistence_data(&players, &matches);

   // Retire le client de toutes les listes d'observateurs.
   for (int i = 0; i < matches.length; i++) {
       int idx;
       vec_find(&matches.data[i].observers, to_remove, idx);
       if (idx != -1) {
           vec_swapsplice(&matches.data[i].observers, idx, 1);
       }
   }

   // Si le client était en partie, on la garde en mémoire pour la prochaine connexion.
   if (was_in_match >= 0 && was_in_match < matches.length) {
       Match *m = &matches.data[was_in_match];

       for (int ci = 0; ci < clients->length; ci++) {
           Client *c = clients->data[ci];
           if (c->active && c != to_remove && c->player && c->player->match == was_in_match) {
               Message msg;
               msg.type = MSG_CHAT;
               snprintf(msg.content, BUF_SIZE - 1,
                        "%s s'est deconnecte (partie en attente de reconnexion)" CRLF,
                        disconnected_name);
               send_client(msg, c->sock);
               break;
           }
       }
       /* Prévient aussi les observateurs. */
       for (int oi = 0; oi < m->observers.length; oi++) {
           Client *obs = m->observers.data[oi];
           if (obs->active) {
               Message obs_msg;
               obs_msg.type = MSG_CHAT;
               snprintf(obs_msg.content, BUF_SIZE - 1,
                        "%s s'est deconnecte (partie en attente de reconnexion)" CRLF,
                        disconnected_name);
               send_client(obs_msg, obs->sock);
           }
       }

       /* La partie reste active : les joueurs restent marqués "en partie"
        * (player->match inchangé) et peuvent se reconnecter. Elle est
        * simplement en pause tant qu'aucun des deux n'est connecté. */
       int someone_left = 0;
       for (int ci = 0; ci < clients->length; ci++) {
           Client *c = clients->data[ci];
           if (c != to_remove && c->active && c->player &&
               c->player->match == was_in_match) {
               someone_left = 1;
               break;
           }
       }
       m->paused = !someone_left;
   }

   //Retire du vec et libère.
   int idx;
   vec_find(clients, to_remove, idx);
   if (idx != -1) {
       vec_swapsplice(clients, idx, 1);
   }
   closesocket(to_remove->sock);
   free(to_remove);
}

void clear_clients(ClientPtrVec *clients)
// fermeture propre des connexions socket
{
   for (int i = 0; i < clients->length; i++) {
      Client *c = clients->data[i];
      if (c->active) {
          closesocket(c->sock);
      }
   }
   save_persistence_data(&players, &matches);
}

void send_message_to_all_clients(ClientPtrVec *clients, Client *sender, const char *buffer, char from_server)
//Envoie un message chat à tous les clients actifs sauf l'expéditeur.
{
   for (int i = 0; i < clients->length; i++) {
      Client *c = clients->data[i];
      if (c->active && c->sock != sender->sock) {
         char message[BUF_SIZE];
         if (from_server == 0) {
            snprintf(message, sizeof message, "%s : %s",
                     sender->player ? sender->player->name : "???", buffer);
         } else {
            snprintf(message, sizeof message, "%s", buffer);
         }
         Message msg;
         msg.type = MSG_CHAT;
         strncpy(msg.content, message, BUF_SIZE - 1);
         msg.content[BUF_SIZE - 1] = '\0';
         send_client(msg, c->sock);
      }
   }
}

void send_list(ClientPtrVec *clients, SOCKET to)
/* Envoie la liste des joueurs en ligne dans un champ structuré. */
{
    Message msg;
    msg.type = MSG_LIST_RESPONSE;
    msg.player_list.count = 0;

    for (int i = 0; i < clients->length; i++) {
        Client *c = clients->data[i];
        if (c->active && c->player &&
            msg.player_list.count < MAX_LIST_NAMES) {
            strncpy(msg.player_list.names[msg.player_list.count],
                    c->player->name, MAX_NOM - 1);
            msg.player_list.names[msg.player_list.count][MAX_NOM - 1] = '\0';
            msg.player_list.count++;
        }
    }

    send_client(msg, to);
}
