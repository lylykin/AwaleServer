#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "server.h"
#include "decodeur.h"

// Initialize saved games count
int saved_game_count = 0;

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
   saved_game_count = 0;
   srand(time(NULL));
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
   int actual = 0;
   int max = sock;
   Client clients[MAX_CLIENTS] = {0};

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);
      FD_SET(STDIN_FILENO, &rdfs);
      FD_SET(sock, &rdfs);

      for(i = 0; i < actual; i++)
      {
         if (clients[i].active) {
             FD_SET(clients[i].sock, &rdfs);
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
         SOCKADDR_IN csin = { 0 };
         socklen_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         Message msg = msg_from_server(csock);
         if(msg.type == -1)
         {
            closesocket(csock);
            continue;
         }

         max = csock > max ? csock : max;
         FD_SET(csock, &rdfs);

         // Find an empty client slot
         int new_client_idx = -1;
         for (int i = 0; i < actual; i++) {
             if (!clients[i].active) {
                 new_client_idx = i;
                 break;
             }
         }
         
         if (new_client_idx == -1 && actual < MAX_CLIENTS) {
             new_client_idx = actual++;
         }
         
         if (new_client_idx == -1 || new_client_idx >= MAX_CLIENTS) {
             closesocket(csock);
             continue;
         }
         
         Client *c = &clients[new_client_idx];
         c->sock = csock;
         c->match = -1;
         c->side = -1;
         c->bio.line_count = 0;
         c->friend_count = 0;
         c->private_mode = 0;
         c->active = 1;
         c->pending[0] = '\0';
         
         if(msg.type == MSG_CONNECT)
         {
             strncpy(c->name, msg.content, MAX_NOM - 1);
             c->name[MAX_NOM - 1] = '\0';
         }
         else
         {
             closesocket(csock);
             continue;
         }

         // If this name is still in an ongoing match, reattach and resume the game
         resume_match(clients, actual, new_client_idx);

         // Notify all clients
         snprintf(buffer, sizeof buffer, "%s s'est connecte !", c->name);
         send_message_to_all_clients(clients, *c, actual, buffer, 1);
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            if(clients[i].active && FD_ISSET(clients[i].sock, &rdfs))
            {
               Message msg = msg_from_server(clients[i].sock);
               Client client = clients[i];
               
               if(msg.type == -1)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  snprintf(buffer, sizeof buffer, "%s s'est deconnecte !", client.name);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else
               {
                   handle_message(clients, actual, i, msg);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void handle_message(Client *clients, int actual, int i, Message msg)
{
    Client *client = &clients[i];
    Message response_msg;
    
    switch (msg.type) {
        case MSG_CHAT:
            send_message_to_all_clients(clients, *client, actual, msg.content, 0);
            break;
            
        case MSG_LIST_REQUEST:
            send_list(clients, actual, client->sock);
            break;
            
        case MSG_SET_BIO: {
            strncpy(client->bio.lines[0], msg.bio.bio_lines[0], MAX_BIO_LENGTH - 1);
            client->bio.line_count = msg.bio.line_count;
            for (int j = 1; j < msg.bio.line_count && j < MAX_BIO_LINES; j++) {
                strncpy(client->bio.lines[j], msg.bio.bio_lines[j], MAX_BIO_LENGTH - 1);
            }
            response_msg.type = MSG_CHAT;
            snprintf(response_msg.content, BUF_SIZE - 1, "Bio saved.");
            send_client(response_msg, client->sock);
            break;
        }
        
        case MSG_GET_BIO_REQUEST: {
            int target_idx = find_client(clients, actual, msg.content);
            if (target_idx == -1) {
                response_msg.type = MSG_CHAT;
                snprintf(response_msg.content, BUF_SIZE - 1, "Joueur %s introuvable.", msg.content);
                send_client(response_msg, client->sock);
                break;
            }
            response_msg.type = MSG_GET_BIO_RESPONSE;
            Client *target = &clients[target_idx];
            for (int j = 0; j < target->bio.line_count && j < MAX_BIO_LINES; j++) {
                strncpy(response_msg.bio.bio_lines[j], target->bio.lines[j], MAX_BIO_LENGTH - 1);
            }
            response_msg.bio.line_count = target->bio.line_count;
            send_client(response_msg, client->sock);
            break;
        }
        
        case MSG_ADD_FRIEND: {
            if (client->friend_count >= MAX_FRIENDS) {
                response_msg.type = MSG_CHAT;
                strncpy(response_msg.content, "Nombre maximum d'amis atteint.", BUF_SIZE - 1);
                send_client(response_msg, client->sock);
                break;
            }
            strncpy(client->friends[client->friend_count], msg.friend_op.friend_name, MAX_NOM - 1);
            client->friends[client->friend_count][MAX_NOM - 1] = '\0';
            client->friend_count++;
            response_msg.type = MSG_CHAT;
            snprintf(response_msg.content, BUF_SIZE - 1, "%s ajoute aux amis.", msg.friend_op.friend_name);
            send_client(response_msg, client->sock);
            break;
        }
        
        case MSG_REMOVE_FRIEND: {
            int found = 0;
            for (int j = 0; j < client->friend_count; j++) {
                if (strcmp(client->friends[j], msg.friend_op.friend_name) == 0) {
                    found = 1;
                    memmove(client->friends + j, client->friends + j + 1, 
                           (client->friend_count - j - 1) * MAX_NOM);
                    client->friend_count--;
                    break;
                }
            }
            if (found) {
                response_msg.type = MSG_CHAT;
                snprintf(response_msg.content, BUF_SIZE - 1, "%s retire des amis.", msg.friend_op.friend_name);
            } else {
                response_msg.type = MSG_CHAT;
                snprintf(response_msg.content, BUF_SIZE - 1, "%s pas dans la liste d'amis.", msg.friend_op.friend_name);
            }
            send_client(response_msg, client->sock);
            break;
        }
        
        case MSG_SET_PRIVATE_MODE: {
            client->private_mode = msg.privacy.private_mode;
            response_msg.type = MSG_CHAT;
            snprintf(response_msg.content, BUF_SIZE - 1, 
                    "Mode prive %s.", client->private_mode ? "active" : "desactive");
            send_client(response_msg, client->sock);
            break;
        }
        
        case MSG_SEND_CHALLENGE: {
            const char *cible = msg.content;
            int idx = find_client(clients, actual, cible);
            
            if (idx == -1) {
                response_msg.type = MSG_CHAT;
                snprintf(response_msg.content, BUF_SIZE - 1, "Joueur %s introuvable.", cible);
                send_client(response_msg, client->sock);
                return;
            }
            
            if (strcmp(cible, client->name) == 0) {
                response_msg.type = MSG_CHAT;
                strncpy(response_msg.content, "Vous ne pouvez pas vous defier vous-meme.", BUF_SIZE - 1);
                send_client(response_msg, client->sock);
                return;
            }
            
            // Check if target has private mode and we're not friends
            Client *target = &clients[idx];
            if (target->private_mode) {
                int is_friend = 0;
                for (int j = 0; j < target->friend_count; j++) {
                    if (strcmp(target->friends[j], client->name) == 0) {
                        is_friend = 1;
                        break;
                    }
                }
                if (!is_friend) {
                    response_msg.type = MSG_CHAT;
                    snprintf(response_msg.content, BUF_SIZE - 1, 
                           "%s a le mode prive active et vous n'etes pas dans sa liste d'amis.", cible);
                    send_client(response_msg, client->sock);
                    return;
                }
            }
            
            strncpy(clients[idx].pending, client->name, MAX_NOM - 1);
            clients[idx].pending[MAX_NOM - 1] = '\0';
            
            response_msg.type = MSG_SEND_CHALLENGE;
            strncpy(response_msg.content, client->name, BUF_SIZE - 1);
            send_client(response_msg, clients[idx].sock);
            
            response_msg.type = MSG_CHAT;
            snprintf(response_msg.content, BUF_SIZE - 1, "Defi envoye a %s.", cible);
            send_client(response_msg, client->sock);
            break;
        }
            
        case MSG_ANSWER_CHALLENGE: {
            char challenger_name[MAX_NOM];
            strncpy(challenger_name, client->pending, MAX_NOM - 1);
            challenger_name[MAX_NOM - 1] = '\0';
            
            if (challenger_name[0] == '\0') {
                return;
            }
            
            int challenger_idx = find_client(clients, actual, challenger_name);
            if (challenger_idx == -1) {
                response_msg.type = MSG_CHAT;
                snprintf(response_msg.content, BUF_SIZE - 1, "Joueur %s introuvable.", challenger_name);
                send_client(response_msg, client->sock);
                client->pending[0] = '\0';
                return;
            }
            
            if (msg.challenge.accepted) {
                start_match(clients, actual, challenger_idx, i);
            } else {
                response_msg.type = MSG_ANSWER_CHALLENGE;
                response_msg.challenge.accepted = 0;
                strncpy(response_msg.challenge.challenger_name, client->name, MAX_NOM - 1);
                strncpy(response_msg.challenge.challenged_name, challenger_name, MAX_NOM - 1);
                send_client(response_msg, clients[challenger_idx].sock);
                client->pending[0] = '\0';
            }
            break;
        }
            
        case MSG_PLAY_TURN:
            if (client->match >= 0) {
                handle_move(clients, actual, i, msg.trou_selected);
            }
            break;
        
        case MSG_LIST_GAMES_REQUEST: {
            response_msg.type = MSG_LIST_GAMES_RESPONSE;
            int count = 0;
            for (int j = 0; j < MAX_MATCHES; j++) {
                if (matches[j].actif) {
                    count++;
                }
            }
            snprintf(response_msg.content, BUF_SIZE - 1, "Parties en cours (%d) :", count);
            for (int j = 0; j < MAX_MATCHES; j++) {
                if (matches[j].actif) {
                    strncat(response_msg.content, 
                           "\n", BUF_SIZE - strlen(response_msg.content) - 1);
                    strncat(response_msg.content, 
                           matches[j].noms[0], BUF_SIZE - strlen(response_msg.content) - 1);
                    strncat(response_msg.content, 
                           " vs ", BUF_SIZE - strlen(response_msg.content) - 1);
                    strncat(response_msg.content, 
                           matches[j].noms[1], BUF_SIZE - strlen(response_msg.content) - 1);
                }
            }
            send_client(response_msg, client->sock);
            break;
        }
        
        case MSG_OBSERVE_GAME: {
            int game_idx = msg.game_info_msg.game_index;
            if (game_idx < 0 || game_idx >= MAX_MATCHES || !matches[game_idx].actif) {
                response_msg.type = MSG_CHAT;
                snprintf(response_msg.content, BUF_SIZE - 1, "Index de partie invalide.");
                send_client(response_msg, client->sock);
                break;
            }
            
            Match *m = &matches[game_idx];
            
            // Check private mode
            if (m->g.joueur >= 0) {
                Client *p1 = NULL;
                Client *p2 = NULL;
                for (int j = 0; j < actual; j++) {
                    if (clients[j].active && clients[j].match == game_idx && clients[j].side == 0) p1 = &clients[j];
                    if (clients[j].active && clients[j].match == game_idx && clients[j].side == 1) p2 = &clients[j];
                }
                if (p1 && p1->private_mode) {
                    int is_friend = 0;
                    for (int j = 0; j < p1->friend_count; j++) {
                        if (strcmp(p1->friends[j], client->name) == 0) {
                            is_friend = 1;
                            break;
                        }
                    }
                    if (!is_friend && strcmp(client->name, p1->name) != 0 && strcmp(client->name, p2->name) != 0) {
                        response_msg.type = MSG_CHAT;
                        snprintf(response_msg.content, BUF_SIZE - 1, 
                               "Impossible d'observer une partie privee.");
                        send_client(response_msg, client->sock);
                        break;
                    }
                }
                if (p2 && p2->private_mode) {
                    int is_friend = 0;
                    for (int j = 0; j < p2->friend_count; j++) {
                        if (strcmp(p2->friends[j], client->name) == 0) {
                            is_friend = 1;
                            break;
                        }
                    }
                    if (!is_friend && strcmp(client->name, p1->name) != 0 && strcmp(client->name, p2->name) != 0) {
                        response_msg.type = MSG_CHAT;
                        snprintf(response_msg.content, BUF_SIZE - 1, 
                               "Impossible d'observer une partie privee.");
                        send_client(response_msg, client->sock);
                        break;
                    }
                }
            }
            
            // Add to observers
            int already_observing = 0;
            for (int j = 0; j < m->observer_count; j++) {
                if (m->observers[j] == client) {
                    already_observing = 1;
                    break;
                }
            }
            if (!already_observing && client->match != game_idx) {
                m->observers[m->observer_count++] = client;
                response_msg.type = MSG_CHAT;
                snprintf(response_msg.content, BUF_SIZE - 1, 
                       "Vous observez maintenant la partie entre %s et %s.", 
                       m->noms[0], m->noms[1]);
                send_client(response_msg, client->sock);
                
                // Send current board state
                response_msg.type = MSG_BOARD_UPDATE;
                memcpy(response_msg.board.board, m->g.plateau, sizeof(m->g.plateau));
                memcpy(response_msg.board.grenier, m->g.grenier, sizeof(m->g.grenier));
                response_msg.board.current_player = m->g.joueur;
                send_client(response_msg, client->sock);
            }
            break;
        }
        
        case MSG_STOP_OBSERVE: {
            int game_idx = msg.game_info_msg.game_index;
            if (game_idx < 0 || game_idx >= MAX_MATCHES) {
                break;
            }
            Match *m = &matches[game_idx];
            for (int j = 0; j < m->observer_count; j++) {
                if (m->observers[j] == client) {
                    memmove(m->observers + j, m->observers + j + 1, 
                           (m->observer_count - j - 1) * sizeof(Client*));
                    m->observer_count--;
                    break;
                }
            }
            response_msg.type = MSG_CHAT;
            snprintf(response_msg.content, BUF_SIZE - 1, "Fin de l'observation de la partie.");
            send_client(response_msg, client->sock);
            break;
        }
        
        case MSG_SAVED_GAMES_REQUEST: {
            response_msg.type = MSG_SAVED_GAMES_RESPONSE;
            if (saved_game_count == 0) {
                strncpy(response_msg.content, "Aucune partie sauvegardee.", BUF_SIZE - 1);
            } else {
                snprintf(response_msg.content, BUF_SIZE - 1, "Parties sauvegardees (%d):", saved_game_count);
                for (int j = 0; j < saved_game_count && j < 10; j++) {
                    strncat(response_msg.content, 
                           "\n", BUF_SIZE - strlen(response_msg.content) - 1);
                    strncat(response_msg.content, 
                           saved_games[j].noms[0], BUF_SIZE - strlen(response_msg.content) - 1);
                    strncat(response_msg.content, 
                           " vs ", BUF_SIZE - strlen(response_msg.content) - 1);
                    strncat(response_msg.content, 
                           saved_games[j].noms[1], BUF_SIZE - strlen(response_msg.content) - 1);
                    strncat(response_msg.content, 
                           " - ", BUF_SIZE - strlen(response_msg.content) - 1);
                    strncat(response_msg.content, 
                           (saved_games[j].winner == -1) ? "Match nul" : 
                           (saved_games[j].winner == 0) ? saved_games[j].noms[0] : saved_games[j].noms[1],
                           BUF_SIZE - strlen(response_msg.content) - 1);
                }
            }
            send_client(response_msg, client->sock);
            break;
        }
        
        default:
            break;
    }
}

// Reattach a client to an ongoing match if its name is still playing,
// and send it everything needed to resume the game.
static void resume_match(Client *clients, int actual, int i)
{
    Client *c = &clients[i];

    for (int k = 0; k < MAX_MATCHES; k++) {
        Match *m = &matches[k];
        if (!m->actif) continue;

        int side = -1;
        if (strcmp(m->noms[0], c->name) == 0) side = m->side_of[0];
        else if (strcmp(m->noms[1], c->name) == 0) side = m->side_of[1];
        if (side < 0) continue;

        c->match = k;
        c->side = side;

        const char *opp_name = (strcmp(m->noms[0], c->name) == 0) ? m->noms[1] : m->noms[0];
        Message msg;

        // GAME_START so the client knows it is back in a game (and not in chat mode)
        msg.type = MSG_GAME_START;
        strncpy(msg.challenge.challenger_name, opp_name, MAX_NOM - 1);
        msg.challenge.challenger_name[MAX_NOM - 1] = '\0';
        strncpy(msg.challenge.challenged_name, c->name, MAX_NOM - 1);
        msg.challenge.challenged_name[MAX_NOM - 1] = '\0';
        msg.challenge.side = side;
        send_client(msg, c->sock);

        // Current board state
        msg.type = MSG_BOARD_UPDATE;
        memcpy(msg.board.board, m->g.plateau, sizeof(m->g.plateau));
        memcpy(msg.board.grenier, m->g.grenier, sizeof(m->g.grenier));
        msg.board.current_player = m->g.joueur;
        send_client(msg, c->sock);

        // Whose turn it is
        msg.type = (m->g.joueur == side) ? MSG_YOUR_TURN : MSG_WAIT;
        send_client(msg, c->sock);

        // Tell the opponent the player is back
        int opp_idx = find_client(clients, actual, opp_name);
        if (opp_idx >= 0) {
            msg.type = MSG_CHAT;
            snprintf(msg.content, BUF_SIZE - 1, "%s s'est reconnecte, la partie reprend." CRLF, c->name);
            send_client(msg, clients[opp_idx].sock);
        }
        return;
    }
}

static void start_match(Client *clients, int actual, int adv_idx, int me_idx)
{
    Client *me = &clients[me_idx];
    Client *adv = &clients[adv_idx];
    Message msg;

    if (me->match >= 0 || adv->match >= 0) {
        msg.type = MSG_CHAT;
        strncpy(msg.content, "Un des joueurs est deja en partie." CRLF, BUF_SIZE - 1);
        send_client(msg, me->sock);
        me->pending[0] = '\0';
        return;
    }

    int k;
    for (k = 0; k < MAX_MATCHES; k++) {
        if (!matches[k].actif) break;
    }
    if (k == MAX_MATCHES) {
        msg.type = MSG_CHAT;
        strncpy(msg.content, "Serveur plein, reessayez plus tard." CRLF, BUF_SIZE - 1);
        send_client(msg, me->sock);
        me->pending[0] = '\0';
        return;
    }

    Match *m = &matches[k];
    m->actif = 1;
    m->observer_count = 0;
    
    // Randomly decide who starts
    int starter = rand() % 2;
    init_game(&m->g, starter);
    
    strncpy(m->noms[0], adv->name, MAX_NOM - 1);
    m->noms[0][MAX_NOM - 1] = '\0';
    strncpy(m->noms[1], me->name, MAX_NOM - 1);
    m->noms[1][MAX_NOM - 1] = '\0';

    adv->match = k;  adv->side = 0;
    me->match = k;   me->side = 1;
    
    // Set actual starter based on random choice
    if (starter == 0) {
        adv->side = 0;
        me->side = 1;
    } else {
        adv->side = 1;
        me->side = 0;
    }

    // Remember each player's side so a reconnecting player can be reattached
    m->side_of[0] = adv->side;
    m->side_of[1] = me->side;
 
    me->pending[0] = '\0';
    adv->pending[0] = '\0';

    msg.type = MSG_GAME_START;
    strncpy(msg.challenge.challenger_name, me->name, MAX_NOM - 1);
    strncpy(msg.challenge.challenged_name, adv->name, MAX_NOM - 1);
    msg.challenge.side = adv->side;
    send_client(msg, adv->sock);
    
    // Also send GAME_START to the challenger
    msg.type = MSG_GAME_START;
    strncpy(msg.challenge.challenger_name, me->name, MAX_NOM - 1);
    strncpy(msg.challenge.challenged_name, adv->name, MAX_NOM - 1);
    msg.challenge.side = me->side;
    send_client(msg, me->sock);

    msg.type = (m->g.joueur == adv->side) ? MSG_YOUR_TURN : MSG_WAIT;
    send_client(msg, adv->sock);

    msg.type = (m->g.joueur == me->side) ? MSG_YOUR_TURN : MSG_WAIT;
    strncpy(msg.challenge.challenger_name, me->name, MAX_NOM - 1);
    strncpy(msg.challenge.challenged_name, adv->name, MAX_NOM - 1);
    msg.challenge.side = me->side;
    send_client(msg, me->sock);

    msg.type = MSG_BOARD_UPDATE;
    memcpy(msg.board.board, m->g.plateau, sizeof(m->g.plateau));
    memcpy(msg.board.grenier, m->g.grenier, sizeof(m->g.grenier));
    msg.board.current_player = m->g.joueur;
    send_client(msg, adv->sock);
    send_client(msg, me->sock);

    msg.type = MSG_CHAT;
    snprintf(msg.content, BUF_SIZE - 1, "Vous etes le joueur %d. Partie demarree contre %s." CRLF, adv->side, me->name);
    send_client(msg, adv->sock);
    snprintf(msg.content, BUF_SIZE - 1, "Vous etes le joueur %d. Partie demarree contre %s." CRLF, me->side, adv->name);
    send_client(msg, me->sock);
}

static void save_game(Match *m, int winner)
{
    if (saved_game_count >= MAX_SAVED_GAMES) {
        // Shift old games
        memmove(&saved_games[0], &saved_games[1], (MAX_SAVED_GAMES - 1) * sizeof(SavedGame));
        saved_game_count--;
    }
    
    SavedGame *sg = &saved_games[saved_game_count++];
    strncpy(sg->noms[0], m->noms[0], MAX_NOM - 1);
    sg->noms[0][MAX_NOM - 1] = '\0';
    strncpy(sg->noms[1], m->noms[1], MAX_NOM - 1);
    sg->noms[1][MAX_NOM - 1] = '\0';
    memcpy(&sg->g, &m->g, sizeof(Game));
    sg->winner = winner;
}

static void handle_move(Client *clients, int actual, int i, int trou)
{
    Client *me = &clients[i];
    Match *m = &matches[me->match];
    Message msg;
    int opp = -1;

    if (me->side != m->g.joueur) {
       msg.type = MSG_NOT_YOUR_TURN;
       send_client(msg, me->sock);
       return;
    }
    
    if (play_move(&m->g, trou) < 0) {
       msg.type = MSG_MOVE_INVALID;
       send_client(msg, me->sock);
       return;
    }
    
    for (int k = 0; k < actual; k++) {
        if (clients[k].active && k != i && clients[k].match == me->match) {
            opp = k;
            break;
        }
    }

    // Send board update to both players and observers
    msg.type = MSG_BOARD_UPDATE;
    memcpy(msg.board.board, m->g.plateau, sizeof(m->g.plateau));
    memcpy(msg.board.grenier, m->g.grenier, sizeof(m->g.grenier));
    msg.board.current_player = m->g.joueur;
    send_client(msg, me->sock);
    if (opp >= 0) send_client(msg, clients[opp].sock);
    
    // Send to observers
    for (int j = 0; j < m->observer_count; j++) {
        if (m->observers[j]->active && m->observers[j] != me && (opp < 0 || m->observers[j] != &clients[opp])) {
            send_client(msg, m->observers[j]->sock);
        }
    }

    if (!end_game(m->g.plateau)) {
       // Index of the player whose turn it is now (may be -1 if disconnected)
       int cur = (m->g.joueur == me->side) ? i : opp;
       int other = (cur == i) ? opp : i;

       msg.type = MSG_YOUR_TURN;
       send_client(msg, clients[cur].sock);

       if (other >= 0) {
          msg.type = MSG_WAIT;
          send_client(msg, clients[other].sock);
       }
       
       msg.type = MSG_CHAT;
       snprintf(msg.content, BUF_SIZE - 1, "%s a joue le trou %d. C'est au tour de %s." CRLF,
                me->name, trou, m->noms[m->g.joueur]);
       send_client(msg, me->sock);
       if (opp >= 0) send_client(msg, clients[opp].sock);
       
       // Notify observers
       for (int j = 0; j < m->observer_count; j++) {
           if (m->observers[j]->active) {
               send_client(msg, m->observers[j]->sock);
           }
       }
    }
    
    if (end_game(m->g.plateau)) {
       msg.type = MSG_GAME_OVER;
       int g0 = m->g.grenier[0], g1 = m->g.grenier[1];
       int winner = -1;
       if (g0 > g1) winner = 0;
       else if (g1 > g0) winner = 1;
       
       if (g0 > g1)
          snprintf(msg.content, BUF_SIZE - 1, "Gagnant : %s (%d - %d)" CRLF, m->noms[0], g0, g1);
       else if (g1 > g0)
          snprintf(msg.content, BUF_SIZE - 1, "Gagnant : %s (%d - %d)" CRLF, m->noms[1], g0, g1);
       else
          snprintf(msg.content, BUF_SIZE - 1, "Match nul (%d - %d)" CRLF, g0, g1);
       send_client(msg, me->sock);
       if (opp >= 0) send_client(msg, clients[opp].sock);
       
       // Notify observers
       for (int j = 0; j < m->observer_count; j++) {
           if (m->observers[j]->active) {
               send_client(msg, m->observers[j]->sock);
           }
       }

       // Save the game
       save_game(m, winner);

       m->actif = 0;
       me->match = -1;
       if (opp >= 0) clients[opp].match = -1;
       
       // Clear observers
       m->observer_count = 0;
    }
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      if (clients[i].active) {
          closesocket(clients[i].sock);
      }
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   if (!clients[to_remove].active) return;
   
   int was_in_match = clients[to_remove].match;
   char disconnected_name[MAX_NOM];
   strncpy(disconnected_name, clients[to_remove].name, MAX_NOM - 1);
   disconnected_name[MAX_NOM - 1] = '\0';
   
   // Remove from all match observers
   for (int i = 0; i < MAX_MATCHES; i++) {
       for (int j = 0; j < matches[i].observer_count; j++) {
           if (matches[i].observers[j] == &clients[to_remove]) {
               memmove(matches[i].observers + j, matches[i].observers + j + 1, 
                      (matches[i].observer_count - j - 1) * sizeof(Client*));
               matches[i].observer_count--;
               break;
           }
       }
   }
   
   // If the client was in a match, keep the match alive so the player can
   // resume it by reconnecting with the same name.
   if (was_in_match >= 0) {
       Match *m = &matches[was_in_match];

       // Notify the other player and the observers (chat only, not GAME_OVER)
       for (int i = 0; i < *actual; i++) {
           if (clients[i].active && i != to_remove && clients[i].match == was_in_match) {
               Message msg;
               msg.type = MSG_CHAT;
               snprintf(msg.content, BUF_SIZE - 1, "%s s'est deconnecte (partie en attente de reconnexion)" CRLF, disconnected_name);
               send_client(msg, clients[i].sock);
               break;
           }
       }
       for (int j = 0; j < m->observer_count; j++) {
           if (m->observers[j]->active) {
               Message obs_msg;
               obs_msg.type = MSG_CHAT;
               snprintf(obs_msg.content, BUF_SIZE - 1, "%s s'est deconnecte (partie en attente de reconnexion)" CRLF, disconnected_name);
               send_client(obs_msg, m->observers[j]->sock);
           }
       }

       // If no player is still connected in this match, close it
       int someone_left = 0;
       for (int i = 0; i < *actual; i++) {
           if (i != to_remove && clients[i].active && clients[i].match == was_in_match) {
               someone_left = 1;
               break;
           }
       }
       if (!someone_left) {
           m->actif = 0;
           m->observer_count = 0;
       }
   }
   
   // Mark the client slot as inactive
   clients[to_remove].active = 0;
   closesocket(clients[to_remove].sock);
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      if(clients[i].active && sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         Message msg;
         msg.type = MSG_CHAT;
         strncpy(msg.content, message, BUF_SIZE - 1);
         msg.content[BUF_SIZE - 1] = '\0';
         send_client(msg, clients[i].sock);
      }
   }
}

static int init_connection()
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

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

static int find_client(Client *clients, int actual, const char *name)
{
    int i;
    for (i = 0; i < actual; i++) {
        if (clients[i].active && strcmp(clients[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void send_list(Client *clients, int actual, SOCKET to)
{
    char message[BUF_SIZE];
    strncpy(message, "Online players:", BUF_SIZE - 1);
    message[BUF_SIZE - 1] = 0;

    for(int i = 0; i < actual; i++)
    {
        if (clients[i].active) {
            strncat(message, " ", sizeof message - strlen(message) - 1);
            strncat(message, clients[i].name, sizeof message - strlen(message) - 1);
        }
    }

    Message msg;
    msg.type = MSG_LIST_RESPONSE;
    strncpy(msg.content, message, BUF_SIZE - 1);
    msg.content[BUF_SIZE - 1] = '\0';
    send_client(msg, to);
}

int main(void)
{
   init();
   app();
   end();
   return EXIT_SUCCESS;
}
