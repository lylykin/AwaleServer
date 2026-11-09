#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "server.h"

/* server.c : point d'entrée et boucle principale.
 *
 * players (défini dans client_manager.c) est un PlayerPtrVec : le magasin
 * persistant des identités, chaque ClientPlayer alloué sur le tas. clients
 * est un ClientPtrVec : chaque connexion alloue un Client sur le tas, lié
 * à un ClientPlayer via ->player. À la déconnexion le Client est retiré
 * du vec et libéré, mais le ClientPlayer conserve toutes les données
 * (bio, amis, match, side). */

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
   srand(time(NULL));
   vec_init(&players);
   vec_init(&matches);
   vec_init(&saved_games);

   load_persistence_data(&players, &matches);
   printf("Loaded %d players and %d matches from persistence.\n",
          players.length, matches.length);
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
   save_persistence_data(&players, &matches);
   vec_deinit(&saved_games);
   printf("Saved persistence data.\n");
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   ClientPtrVec clients;
   vec_init(&clients);
   int max = sock;

   fd_set rdfs;

   while(1)
   {
      FD_ZERO(&rdfs);
      FD_SET(STDIN_FILENO, &rdfs);
      FD_SET(sock, &rdfs);

      for (int ci = 0; ci < clients.length; ci++) {
         Client *c = clients.data[ci];
         if (c->active) {
             FD_SET(c->sock, &rdfs);
             if (c->sock > max) max = c->sock;
         }
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* Nouvelle connexion */
         SOCKADDR_IN csin = { 0 };
         socklen_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         Message msg = msg_from_server(csock);
         if(msg.type == MSG_ERROR)
         {
            closesocket(csock);
            continue;
         }

         max = csock > max ? csock : max;

         if (msg.type != MSG_CONNECT) {
             closesocket(csock);
             continue;
         }

         char incoming_name[MAX_NOM];
         strncpy(incoming_name, msg.content, MAX_NOM - 1);
         incoming_name[MAX_NOM - 1] = '\0';

         /* Cherche l'identité persistante. */
         ClientPlayer *player = find_persistent_player(&players, incoming_name);

         /* Nouveau joueur : alloue sur le tas et pousse dans le vec. */
         if (!player && players.length < MAX_CLIENTS) {
             player = calloc(1, sizeof(ClientPlayer));
             if (player) {
                 vec_init(&player->bio.lines);
                 vec_init(&player->friends);
                 player->match = -1;
                 player->side = -1;
                 strncpy(player->name, incoming_name, MAX_NOM - 1);
                 player->name[MAX_NOM - 1] = '\0';
                 vec_push(&players, player);
             }
         }

         if (!player) {
             closesocket(csock);
             continue;
         }

         /* Alloue un nouveau Client et le lie au ClientPlayer. */
         Client *nc = calloc(1, sizeof(Client));
         if (!nc) {
             closesocket(csock);
             continue;
         }
         nc->sock = csock;
         nc->active = 1;
         nc->player = player;
         nc->pending[0] = '\0';
         vec_push(&clients, nc);

         /* Rattache à une partie en cours si reconnexion. */
         resume_match(&clients, nc);

         snprintf(buffer, sizeof buffer, "%s s'est connecte !", nc->player->name);
         send_message_to_all_clients(&clients, nc, buffer, 1);

         save_persistence_data(&players, &matches);
      }
      else
      {
         /* Un client existant a envoyé des données ou s'est déconnecté.
          * On itère sur une copie des pointeurs car remove_client modifie le vec. */
         int n = clients.length;
         for (int i = 0; i < n; i++) {
             Client *c = clients.data[i];
             if (!c || !c->active || !FD_ISSET(c->sock, &rdfs)) continue;

             Message msg = msg_from_server(c->sock);
             if(msg.type == MSG_ERROR)
             {
                closesocket(c->sock);
                char name[MAX_NOM] = "???";
                if (c->player) {
                    strncpy(name, c->player->name, MAX_NOM - 1);
                    name[MAX_NOM - 1] = '\0';
                }
                remove_client(&clients, c);
                snprintf(buffer, sizeof buffer, "%s s'est deconnecte !", name);
                /* après remove_client, c est libéré - on passe un client factice */
                Client fake = { .sock = -1, .active = 0, .player = NULL };
                send_message_to_all_clients(&clients, &fake, buffer, 1);
             }
             else
             {
                handle_message(&clients, c, msg);
             }
             break;
         }
      }
   }

   clear_clients(&clients);
   /* Libère les Client restants dans le vec. */
   {
       for (int i = 0; i < clients.length; i++) {
           free(clients.data[i]);
       }
       vec_deinit(&clients);
   }
   end_connection(sock);
}

int main(void)
{
   init();
   app();
   end();
   return EXIT_SUCCESS;
}
