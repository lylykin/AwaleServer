#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "client.h"
#include "../shared/decodeur.h"

static char challenger[MAX_NOM] = "";
static int observing_game = -1;

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
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

void show_commands(const char* name)
{

    printf("==========================\nBienvenue %s ! \n\n", name);
     printf("Commandes disponibles :\n");
     printf("  * /cmd - afficher la liste des commandes disponibles\n");
     printf("  * /list - afficher les joueurs en ligne\n");
     printf("  * /challenge <joueur> - defier un joueur\n");
     printf("  * /games - lister les parties en cours\n");
     printf("  * /observe <index> - observer une partie\n");
     printf("  * /stopobserve - arreter d'observer\n");
     printf("  * /bio - definir votre bio\n");
     printf("  * /bio <joueur> - voir la bio d'un joueur\n");
     printf("  * /friend add <joueur> - ajouter un ami\n");
     printf("  * /friend remove <joueur> - retirer un ami\n");
     printf("  * /private on - activer le mode prive\n");
     printf("  * /private off - desactiver le mode prive\n");
     printf("  * /saved - lister les parties sauvegardees\n");
     printf("Lors d'une partie : entrez un nombre (1-6) ou /move <1-6> pour sélectionner une case\n");
     printf("==========================\n\n");

}
static void app(const char *address, const char *name)
{
   SOCKET sock = init_connection(address);
   char buffer[BUF_SIZE];

   show_commands(name);
   fd_set rdfs;

   // Send connection message with name
   Message msg;
   msg.type = MSG_CONNECT;
   strncpy(msg.content, name, BUF_SIZE - 1);
   msg.content[BUF_SIZE - 1] = '\0';
   send_server(msg, sock);

   while(1)
   {
      FD_ZERO(&rdfs);
      FD_SET(STDIN_FILENO, &rdfs);
      FD_SET(sock, &rdfs);

      if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         if (fgets(buffer, BUF_SIZE - 1, stdin) == NULL)
         {
            break;
         }
         char *p = strstr(buffer, "\n");
         if(p != NULL)
         {
            *p = 0;
         }
         else
         {
            buffer[BUF_SIZE - 1] = 0;
         }

         // Handle challenge response
         if (challenger[0] != '\0' && (strcmp(buffer, "o") == 0 || strcmp(buffer, "n") == 0)) {
            msg.type = MSG_ANSWER_CHALLENGE;
            msg.challenge.accepted = (strcmp(buffer, "o") == 0) ? 1 : 0;
            strncpy(msg.challenge.challenger_name, challenger, MAX_NOM - 1);
            msg.challenge.challenger_name[MAX_NOM - 1] = '\0';
            strncpy(msg.challenge.challenged_name, name, MAX_NOM - 1);
            msg.challenge.challenged_name[MAX_NOM - 1] = '\0';
            send_server(msg, sock);
            challenger[0] = '\0';
         }
         else if (strncmp(buffer, "/challenge ", strlen("/challenge ")) == 0)
         {
            const char *adversaire = buffer + strlen("/challenge ");
            if (*adversaire == '\0')
            {
               printf("Utilisation : /challenge <joueur>\n");
            }
            else
            {
               msg.type = MSG_SEND_CHALLENGE;
               strncpy(msg.content, adversaire, BUF_SIZE - 1);
               msg.content[BUF_SIZE - 1] = '\0';
               send_server(msg, sock);
            }
         }
         else if (strcmp(buffer, "/list") == 0)
         {
            msg.type = MSG_LIST_REQUEST;
            send_server(msg, sock);
         }
         else if (strcmp(buffer, "/cmd") == 0)
         {
             show_commands(name);
         }
         else if (strcmp(buffer, "/games") == 0)
         {
            msg.type = MSG_LIST_GAMES_REQUEST;
            send_server(msg, sock);
         }
         else if (strncmp(buffer, "/observe ", strlen("/observe ")) == 0)
         {
            const char *game_str = buffer + strlen("/observe ");
            int game_idx = atoi(game_str);
            msg.type = MSG_OBSERVE_GAME;
            msg.game_op.game_index = game_idx;
            send_server(msg, sock);
            observing_game = game_idx;
         }
         else if (strcmp(buffer, "/stopobserve") == 0)
         {
            msg.type = MSG_STOP_OBSERVE;
            msg.game_op.game_index = observing_game;
            send_server(msg, sock);
            observing_game = -1;
         }
         else if (strcmp(buffer, "/bio") == 0)
         {
            printf("Entrez votre bio (max 10 lignes, ligne vide pour terminer) :\n");
            Message bio_msg;
            bio_msg.type = MSG_SET_BIO;
            bio_msg.bio.line_count = 0;
            for (int j = 0; j < 10; j++) {
                if (fgets(buffer, BUF_SIZE - 1, stdin) == NULL) break;
                p = strstr(buffer, "\n");
                if (p) *p = 0;
                if (buffer[0] == '\0') break;
                strncpy(bio_msg.bio.bio_lines[j], buffer, 255);
                bio_msg.bio.bio_lines[j][255] = '\0';
                bio_msg.bio.line_count++;
            }
            send_server(bio_msg, sock);
         }
         else if (strncmp(buffer, "/bio ", strlen("/bio ")) == 0)
         {
            const char *player = buffer + strlen("/bio ");
            msg.type = MSG_GET_BIO_REQUEST;
            strncpy(msg.content, player, BUF_SIZE - 1);
            msg.content[BUF_SIZE - 1] = '\0';
            send_server(msg, sock);
         }
         else if (strncmp(buffer, "/friend add ", strlen("/friend add ")) == 0)
         {
            const char *friend_name = buffer + strlen("/friend add ");
            msg.type = MSG_ADD_FRIEND;
            strncpy(msg.friend_op.friend_name, friend_name, MAX_NOM - 1);
            msg.friend_op.friend_name[MAX_NOM - 1] = '\0';
            send_server(msg, sock);
         }
         else if (strncmp(buffer, "/friend remove ", strlen("/friend remove ")) == 0)
         {
            const char *friend_name = buffer + strlen("/friend remove ");
            msg.type = MSG_REMOVE_FRIEND;
            strncpy(msg.friend_op.friend_name, friend_name, MAX_NOM - 1);
            msg.friend_op.friend_name[MAX_NOM - 1] = '\0';
            send_server(msg, sock);
         }
         else if (strcmp(buffer, "/private on") == 0)
         {
            msg.type = MSG_SET_PRIVATE_MODE;
            msg.privacy.private_mode = 1;
            send_server(msg, sock);
         }
         else if (strcmp(buffer, "/private off") == 0)
         {
            msg.type = MSG_SET_PRIVATE_MODE;
            msg.privacy.private_mode = 0;
            send_server(msg, sock);
         }
         else if (strncmp(buffer, "/move ", strlen("/move ")) == 0)
         {
            // Handle /move <number> command
            const char *move_str = buffer + strlen("/move ");
            char *endptr;
            long num = strtol(move_str, &endptr, 10);
            if (endptr == move_str + strlen(move_str) && num >= 1 && num <= 6) {
                msg.type = MSG_PLAY_TURN;
                msg.trou_selected = (int)num;
                send_server(msg, sock);
            } else {
                msg.type = MSG_CHAT;
                strncpy(msg.content, buffer, BUF_SIZE - 1);
                msg.content[BUF_SIZE - 1] = '\0';
                send_server(msg, sock);
            }
         }
         else if (strcmp(buffer, "/saved") == 0)
         {
            msg.type = MSG_SAVED_GAMES_REQUEST;
            send_server(msg, sock);
         }
         else
         {
            // A bare number (1-6) is always sent as a move attempt; the
            // server plays it if we are in a game, or relays it as chat.
            char *endptr;
            long num = strtol(buffer, &endptr, 10);
            if (endptr == buffer + strlen(buffer) && num >= 1 && num <= 6) {
                msg.type = MSG_PLAY_TURN;
                msg.trou_selected = (int)num;
                send_server(msg, sock);
            } else {
                msg.type = MSG_CHAT;
                strncpy(msg.content, buffer, BUF_SIZE - 1);
                msg.content[BUF_SIZE - 1] = '\0';
                send_server(msg, sock);
            }
         }
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         Message received_msg = msg_from_server(sock);

         if(received_msg.type == MSG_ERROR)
         {
            printf("Server disconnected!\n");
            break;
         }

         handle_msg_server(sock, received_msg);

         // Remember our side so the board is shown with our row at the bottom
         if (received_msg.type == MSG_GAME_START) {
            set_player_side(received_msg.challenge.side);
            /* Détermine le nom de l'adversaire : celui qui n'est pas nous */
            const char *opp = (strcmp(received_msg.challenge.challenger_name, name) == 0)
                            ? received_msg.challenge.challenged_name
                            : received_msg.challenge.challenger_name;
            set_game_names(name, received_msg.challenge.side, opp);
         } else if (received_msg.type == MSG_GAME_OVER) {
            set_player_side(-1);
            observing_game = -1;
         }

         if (received_msg.type == MSG_SEND_CHALLENGE) {
            snprintf(challenger, sizeof challenger, "%s",
                     received_msg.challenge.challenger_name);
         }
      }
   }

   end_connection(sock);
}

static int init_connection(const char *address)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };
   struct hostent *hostinfo;

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   hostinfo = gethostbyname(address);
   if (hostinfo == NULL)
   {
      fprintf (stderr, "Unknown host %s.\n", address);
      exit(EXIT_FAILURE);
   }

   sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
   {
      perror("connect()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

int main(int argc, char **argv)
{
   if(argc < 3)
   {
      printf("Utilisation : %s [adresse] [pseudo]\n", argv[0]);
      return EXIT_FAILURE;
   }

   init();
   app(argv[1], argv[2]);
   end();

   return EXIT_SUCCESS;
}
