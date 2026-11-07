#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "server.h"

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

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         socklen_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { 0 };
         c.sock = csock;
         c.match = -1;   /* sinon 0 = "dans la partie 0" */
         strncpy(c.name, buffer, MAX_NOM - 1);
         clients[actual] = c;
         actual++;
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else if(strcmp(buffer, "/list") == 0)
               {
                    send_list(clients, actual, client.sock);
               }
               else if (strncmp(buffer, "CHALLENGE ", 10)==0){
                   send_challenge(clients, actual, client, buffer);
               }
               else if (clients[i].match >=0){
                    /* accepte "MOVE 6", "Move 6", "move 6" ou simplement "6" */
                    int trou = 0, ok = 0;
                    char mot[8] = "";
                    if (sscanf(buffer, "%d", &trou) == 1){
                       ok = 1;
                    }
                    else if (sscanf(buffer, "%7s %d", mot, &trou) == 2){
                       for (char *p = mot; *p; p++) *p = (char) tolower((unsigned char) *p);
                       ok = (strcmp(mot, "move") == 0);
                    }
                    if (ok){
                       handle_move(clients, actual, i, trou);
                    }
                    else{
                       write_client(client.sock, "Commande inconnue. Tapez MOVE <1-6> (ou juste le numero)." CRLF);
                    }
               }
               else if (strncmp(buffer, "ACCEPT ", 7) == 0){
                    start_match(clients, actual, i, buffer);
               }
               else if (strncmp(buffer, "REFUSE ", 7)==0){
                    clients[i].pending[0] = '\0';
                    send_response(clients, actual, client, buffer);
               }
               else{
                   send_message_to_all_clients(clients, client, actual, buffer, 0);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}
/* Appelée quand clients[i] envoie "ACCEPT <challenger>".
 * Crée une partie entre le challenger (camp 0, commence) et clients[i] (camp 1). */
static void start_match(Client *clients, int actual, int i, const char *buffer){
    Client *me = &clients[i];
    const char *cible = buffer + strlen("ACCEPT ");
    char msg[BUF_SIZE];

    /* 1. le challenger existe-t-il encore ? */
    int idx = find_client(clients, actual, cible);
    if (idx == -1){
        snprintf(msg, sizeof msg, "Joueur %s introuvable." CRLF, cible);
        write_client(me->sock, msg);
        return;
    }
    Client *adv = &clients[idx];

    /* 2. m'a-t-il vraiment défié ? (on n'accepte pas un défi inventé) */
    if (strcmp(me->pending, cible) != 0){
        snprintf(msg, sizeof msg, "Aucun défi de %s en attente." CRLF, cible);
        write_client(me->sock, msg);
        return;
    }

    /* 3. personne ne doit déjà être en partie */
    if (me->match >= 0 || adv->match >= 0){
        write_client(me->sock, "Un des joueurs est déjà en partie." CRLF);
        me->pending[0] = '\0';
        return;
    }

    /* 4. trouver une place libre dans matches[] */
    int k;
    for (k = 0; k < MAX_MATCHES; k++){
        if (!matches[k].actif) break;
    }
    if (k == MAX_MATCHES){
        write_client(me->sock, "Serveur plein, réessayez plus tard." CRLF);
        return;
    }

    /* 5. initialiser la partie */
    Match *m = &matches[k];
    m->actif = 1;
    init_game(&m->g, 0);                 /* le camp 0 commence */
    strncpy(m->noms[0], adv->name, MAX_NOM - 1);
    m->noms[0][MAX_NOM - 1] = '\0';
    strncpy(m->noms[1], me->name, MAX_NOM - 1);
    m->noms[1][MAX_NOM - 1] = '\0';

    adv->match = k;  adv->side = 0;      /* le challenger joue en premier */
    me->match  = k;  me->side  = 1;
    me->pending[0] = '\0';

    /* 6. prévenir les deux joueurs */
    snprintf(msg, sizeof msg, "ACCEPTED %s" CRLF, me->name);
    write_client(adv->sock, msg);

    snprintf(msg, sizeof msg, "GAME_START %s 0" CRLF "YOUR_TURN" CRLF, me->name);
    write_client(adv->sock, msg);

    snprintf(msg, sizeof msg, "GAME_START %s 1" CRLF "WAIT" CRLF, adv->name);
    write_client(me->sock, msg);

    /* 7. envoyer le plateau initial aux deux joueurs */
    board_to_string(&m->g, msg, sizeof msg);
    write_client(adv->sock, msg);
    write_client(me->sock, msg);

    write_client(adv->sock, "Vous etes le joueur 0. Tapez MOVE <1-6> pour jouer." CRLF);
    write_client(me->sock,  "Vous etes le joueur 1. Attendez le coup de l'adversaire." CRLF);
}

static void handle_move(Client *clients,int actual,int i,int trou){
    Client *me = &clients[i];
    Match *m = &matches[me->match];
    if (me->side != m->g.joueur){
       write_client(me->sock, "Ce n'est pas ton tour."CRLF);
       return;
    }
    if (play_move(&m->g, trou) < 0 ){
       write_client(me->sock, "Coup invalide."CRLF);
       return;
    }
    int opp = -1;
    for (int k = 0; k < actual;k++){
        if (k!=i && clients[k].match == me->match){
            opp = k;
            break;
        }
    }
    char msg[BUF_SIZE];
    board_to_string(&m->g, msg, sizeof msg);
    write_client(me->sock,msg);
    if (opp >= 0) write_client(clients[opp].sock, msg);

    if (!end_game(m->g.plateau)){
       snprintf(msg, sizeof msg, "%s a joue le trou %d. C'est au tour de %s." CRLF,
                me->name, trou, m->noms[m->g.joueur]);
       write_client(me->sock, msg);
       if (opp >= 0) write_client(clients[opp].sock, msg);
    }
    if (end_game(m->g.plateau)){
       /* annoncer le gagnant (version réseau de winner() dans game.c) */
       int g0 = m->g.grenier[0], g1 = m->g.grenier[1];
       if (g0 > g1)
          snprintf(msg, sizeof msg, "GAME_OVER Gagnant : %s (%d - %d)" CRLF, m->noms[0], g0, g1);
       else if (g1 > g0)
          snprintf(msg, sizeof msg, "GAME_OVER Gagnant : %s (%d - %d)" CRLF, m->noms[1], g0, g1);
       else
          snprintf(msg, sizeof msg, "GAME_OVER Match nul (%d - %d)" CRLF, g0, g1);
       write_client(me->sock, msg);
       if (opp >= 0) write_client(clients[opp].sock, msg);

       m->actif = 0;
       me->match = -1;
       if (opp>=0) clients[opp].match = -1;
    }
}
static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}
static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   /* permet de relancer le serveur tout de suite (sinon "Address already in use") */
   int opt = 1;
   setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &opt, sizeof opt);

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}
static int find_client(Client *clients, int actual, const char *name){
      int i;
      for (i = 0;i< actual;i++){
          if (strcmp(clients[i].name, name) == 0){
              return i;
          }
      }
      return -1;
}
static void send_challenge(Client *clients, int actual, Client sender, const char *buffer){
    char message[BUF_SIZE];
    const char *cible = buffer + strlen("CHALLENGE ");
    int idx;
    if (strcmp(cible,sender.name)==0){
        write_client(sender.sock,"Vous ne pouvez pas vous défier vous-meme");
        return;
    }
    idx = find_client(clients, actual, cible);
    if (idx == -1){
        snprintf(message, sizeof message, "Joueur %s introuvable.", cible);
        write_client(sender.sock, message);
        return;
    }
    /* on mémorise qui a défié la cible, pour vérifier l'ACCEPT plus tard */
    strncpy(clients[idx].pending, sender.name, MAX_NOM - 1);
    clients[idx].pending[MAX_NOM - 1] = '\0';
    snprintf(message, sizeof message, "CHALLENGE_FROM %s", sender.name);
    write_client(clients[idx].sock, message);
    snprintf(message, sizeof message, "Défi envoyé à %s.", cible);
    write_client(sender.sock,message);
}
static void send_response(Client *clients, int actual, Client sender, const char *buffer){
    char message[BUF_SIZE];
    const char *result;
    const char *cible;
    int idx;
    if (strncmp(buffer,"ACCEPT ", strlen("ACCEPT "))==0){
        result = "ACCEPTED";
        cible = buffer + strlen("ACCEPT ");
    }
    else {
        result = "REFUSED";
        cible = buffer + strlen("REFUSE ");
    }
    idx = find_client(clients, actual, cible);
    if (idx == -1){
        snprintf(message, sizeof message, "Joueur %s introuvable.", cible);
        write_client(sender.sock, message);
        return;
    }
    snprintf(message, sizeof message, "%s %s", result, sender.name);
    write_client(clients[idx].sock, message);
}
static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}
static void send_list(Client *clients, int actual, SOCKET to)
  {
     char message[BUF_SIZE];
     strncpy(message, "Online:", BUF_SIZE - 1);
     message[BUF_SIZE - 1] = 0;

     for(int i = 0; i < actual; i++)
     {
        strncat(message, " ", sizeof message - strlen(message) - 1);
        strncat(message, clients[i].name, sizeof message -
  strlen(message) - 1);
     }

     write_client(to, message);
  }
int main(void)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
