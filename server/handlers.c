#include <stdio.h>
#include <string.h>

#include "handlers.h"
#include "client_manager.h"
#include "match_manager.h"
#include "persistence.h"

/* Sous-gestionnaires, regroupés par domaine */

/* Chat et liste des joueurs */

static void handle_chat(ClientPtrVec *clients, Client *client, const char *content) {
    send_message_to_all_clients(clients, client, content, 0);
}

static void handle_list_request(ClientPtrVec *clients, Client *client) {
    send_list(clients, client->sock);
}

/* Bio */

static void handle_set_bio(Client *client, const Message *msg) {
    Message resp;
    ClientPlayer *p = client->player;

    vec_clear(&p->bio.lines);
    int n = msg->bio.line_count < MAX_BIO_LINES ? msg->bio.line_count : MAX_BIO_LINES;
    for (int j = 0; j < n; j++) {
        BioLine line;
        strncpy(line.value, msg->bio.bio_lines[j], MAX_BIO_LENGTH - 1);
        line.value[MAX_BIO_LENGTH - 1] = '\0';
        vec_push(&p->bio.lines, line);
    }

    resp.type = MSG_CHAT;
    snprintf(resp.content, BUF_SIZE - 1, "Bio saved.");
    send_client(resp, client->sock);
    save_persistence_data(&players, &matches);
}

static void handle_get_bio(ClientPtrVec *clients, Client *client, const char *target_name) {
    (void)clients;
    Message resp;
    Client *target = find_client(clients, target_name);
    if (!target) {
        resp.type = MSG_CHAT;
        snprintf(resp.content, BUF_SIZE - 1, "Joueur %s introuvable.", target_name);
        send_client(resp, client->sock);
        return;
    }
    resp.type = MSG_GET_BIO_RESPONSE;
    int n = target->player->bio.lines.length < MAX_BIO_LINES
          ? target->player->bio.lines.length : MAX_BIO_LINES;
    for (int j = 0; j < n; j++) {
        strncpy(resp.bio.bio_lines[j], target->player->bio.lines.data[j].value,
                MAX_BIO_LENGTH - 1);
        resp.bio.bio_lines[j][MAX_BIO_LENGTH - 1] = '\0';
    }
    resp.bio.line_count = n;
    send_client(resp, client->sock);
}

/* Amis */

static void handle_add_friend(Client *client, const char *friend_name) {
    Message resp;
    ClientPlayer *p = client->player;
    if (p->friends.length >= MAX_FRIENDS) {
        resp.type = MSG_CHAT;
        strncpy(resp.content, "Nombre maximum d'amis atteint.", BUF_SIZE - 1);
        resp.content[BUF_SIZE - 1] = '\0';
        send_client(resp, client->sock);
        return;
    }
    FriendName fname;
    strncpy(fname.value, friend_name, MAX_NOM - 1);
    fname.value[MAX_NOM - 1] = '\0';
    vec_push(&p->friends, fname);

    resp.type = MSG_CHAT;
    snprintf(resp.content, BUF_SIZE - 1, "%s ajoute aux amis.", friend_name);
    send_client(resp, client->sock);
    save_persistence_data(&players, &matches);
}

static void handle_remove_friend(Client *client, const char *friend_name) {
    Message resp;
    ClientPlayer *p = client->player;
    int idx = -1;
    for (int j = 0; j < p->friends.length; j++) {
        if (strcmp(p->friends.data[j].value, friend_name) == 0) {
            idx = j;
            break;
        }
    }

    if (idx != -1) {
        vec_swapsplice(&p->friends, idx, 1);
        resp.type = MSG_CHAT;
        snprintf(resp.content, BUF_SIZE - 1, "%s retire des amis.", friend_name);
    } else {
        resp.type = MSG_CHAT;
        snprintf(resp.content, BUF_SIZE - 1, "%s pas dans la liste d'amis.", friend_name);
    }
    send_client(resp, client->sock);
    save_persistence_data(&players, &matches);
}

/* Mode privé */

static void handle_private_mode(Client *client, int enabled) {
    Message resp;
    client->player->private_mode = enabled;
    resp.type = MSG_CHAT;
    snprintf(resp.content, BUF_SIZE - 1, "Mode prive %s.",
             client->player->private_mode ? "active" : "desactive");
    send_client(resp, client->sock);
    save_persistence_data(&players, &matches);
}

/* Défis */

static int is_friend_of(const ClientPlayer *target, const char *name) {
    for (int j = 0; j < target->friends.length; j++) {
        if (strcmp(target->friends.data[j].value, name) == 0) return 1;
    }
    return 0;
}

static void handle_send_challenge(ClientPtrVec *clients, Client *client, const char *target_name) {
    Message resp;
    Client *target = find_client(clients, target_name);

    if (!target) {
        resp.type = MSG_CHAT;
        snprintf(resp.content, BUF_SIZE - 1, "Joueur %s introuvable.", target_name);
        send_client(resp, client->sock);
        return;
    }
    if (strcmp(target_name, client->player->name) == 0) {
        resp.type = MSG_CHAT;
        strncpy(resp.content, "Vous ne pouvez pas vous defier vous-meme.", BUF_SIZE - 1);
        resp.content[BUF_SIZE - 1] = '\0';
        send_client(resp, client->sock);
        return;
    }

    if (target->player->private_mode && !is_friend_of(target->player, client->player->name)) {
        resp.type = MSG_CHAT;
        snprintf(resp.content, BUF_SIZE - 1,
               "%s a le mode prive active et vous n'etes pas dans sa liste d'amis.", target_name);
        send_client(resp, client->sock);
        return;
    }

    strncpy(target->pending, client->player->name, MAX_NOM - 1);
    target->pending[MAX_NOM - 1] = '\0';

    resp.type = MSG_SEND_CHALLENGE;
    strncpy(resp.challenge.challenger_name, client->player->name, MAX_NOM - 1);
    resp.challenge.challenger_name[MAX_NOM - 1] = '\0';
    send_client(resp, target->sock);

    resp.type = MSG_CHAT;
    snprintf(resp.content, BUF_SIZE - 1, "Defi envoye a %s.", target_name);
    send_client(resp, client->sock);
}

static void handle_answer_challenge(ClientPtrVec *clients, Client *client, const Message *msg) {
    Message resp;
    char challenger_name[MAX_NOM];
    strncpy(challenger_name, client->pending, MAX_NOM - 1);
    challenger_name[MAX_NOM - 1] = '\0';

    if (challenger_name[0] == '\0') return;

    Client *challenger = find_client(clients, challenger_name);
    if (!challenger) {
        resp.type = MSG_CHAT;
        snprintf(resp.content, BUF_SIZE - 1, "Joueur %s introuvable.", challenger_name);
        send_client(resp, client->sock);
        client->pending[0] = '\0';
        return;
    }

    if (msg->challenge.accepted) {
        start_match(clients, challenger, client);
    } else {
        resp.type = MSG_ANSWER_CHALLENGE;
        resp.challenge.accepted = 0;
        strncpy(resp.challenge.challenger_name, client->player->name, MAX_NOM - 1);
        resp.challenge.challenger_name[MAX_NOM - 1] = '\0';
        strncpy(resp.challenge.challenged_name, challenger_name, MAX_NOM - 1);
        resp.challenge.challenged_name[MAX_NOM - 1] = '\0';
        send_client(resp, challenger->sock);
        client->pending[0] = '\0';
    }
}

/* Coups de jeu */

static void handle_play_turn(ClientPtrVec *clients, Client *client, int trou) {
    if (client->player->match >= 0) {
        handle_move(clients, client, trou);
    } else {
        char chat[BUF_SIZE];
        snprintf(chat, sizeof chat, "%d", trou);
        send_message_to_all_clients(clients, client, chat, 0);
    }
}

/* Liste des parties et observation */

static void handle_list_games(Client *client) {
    Message resp;
    resp.type = MSG_LIST_GAMES_RESPONSE;
    resp.game_list.count = 0;
    for (int j = 0; j < matches.length; j++) {
        if (matches.data[j].actif && resp.game_list.count < MAX_GAMES_IN_MSG) {
            strncpy(resp.game_list.games[resp.game_list.count].player1,
                    matches.data[j].noms[0], MAX_NOM - 1);
            resp.game_list.games[resp.game_list.count].player1[MAX_NOM - 1] = '\0';
            strncpy(resp.game_list.games[resp.game_list.count].player2,
                    matches.data[j].noms[1], MAX_NOM - 1);
            resp.game_list.games[resp.game_list.count].player2[MAX_NOM - 1] = '\0';
            resp.game_list.count++;
        }
    }
    send_client(resp, client->sock);
}

static void handle_observe_game(ClientPtrVec *clients, Client *client, int game_idx) {
    Message resp;
    if (game_idx < 0 || game_idx >= matches.length || !matches.data[game_idx].actif) {
        resp.type = MSG_CHAT;
        snprintf(resp.content, BUF_SIZE - 1, "Index de partie invalide.");
        send_client(resp, client->sock);
        return;
    }

    Match *m = &matches.data[game_idx];

    /* Mode privé : l'observateur doit être ami d'un des joueurs. */
    Client *p1 = NULL, *p2 = NULL;
    {
        for (int ci = 0; ci < clients->length; ci++) {
            Client *c = clients->data[ci];
            if (c->active && c->player && c->player->match == game_idx) {
                if (c->player->side == 0) p1 = c;
                else if (c->player->side == 1) p2 = c;
            }
        }
    }
    Client *players_arr[2] = {p1, p2};
    for (int p = 0; p < 2; p++) {
        if (players_arr[p] && players_arr[p]->player->private_mode) {
            if (!is_friend_of(players_arr[p]->player, client->player->name) &&
                strcmp(client->player->name,
                       p1 && p1->player ? p1->player->name : "") != 0 &&
                strcmp(client->player->name,
                       p2 && p2->player ? p2->player->name : "") != 0) {
                resp.type = MSG_CHAT;
                snprintf(resp.content, BUF_SIZE - 1,
                       "Impossible d'observer une partie privee.");
                send_client(resp, client->sock);
                return;
            }
        }
    }

    int idx;
    vec_find(&m->observers, client, idx);
    if (idx == -1 && client->player->match != game_idx) {
        vec_push(&m->observers, client);
        resp.type = MSG_CHAT;
        snprintf(resp.content, BUF_SIZE - 1,
               "Vous observez maintenant la partie entre %s et %s.",
               m->noms[0], m->noms[1]);
        send_client(resp, client->sock);

        resp.type = MSG_BOARD_UPDATE;
        memcpy(resp.board.board, m->g.plateau, sizeof(m->g.plateau));
        memcpy(resp.board.grenier, m->g.grenier, sizeof(m->g.grenier));
        resp.board.current_player = m->g.joueur;
        send_client(resp, client->sock);
    }
}

static void handle_stop_observe(Client *client, int game_idx) {
    Message resp;
    if (game_idx < 0 || game_idx >= matches.length) return;
    Match *m = &matches.data[game_idx];

    int idx;
    vec_find(&m->observers, client, idx);
    if (idx != -1) {
        vec_swapsplice(&m->observers, idx, 1);
    }
    resp.type = MSG_CHAT;
    snprintf(resp.content, BUF_SIZE - 1, "Fin de l'observation de la partie.");
    send_client(resp, client->sock);
}

/* Parties sauvegardées - envoyées dans un champ structuré, pas du texte. */

static void handle_saved_games_request(Client *client) {
    Message resp;
    resp.type = MSG_SAVED_GAMES_RESPONSE;
    resp.saved_games_list.count = 0;

    int limit = saved_games.length < MAX_SAVED_IN_MSG
              ? saved_games.length : MAX_SAVED_IN_MSG;
    for (int j = 0; j < limit; j++) {
        SavedGame *sg = &saved_games.data[j];
        SavedGameInfo *info = &resp.saved_games_list.games[j];
        strncpy(info->noms[0], sg->noms[0], MAX_NOM - 1);
        info->noms[0][MAX_NOM - 1] = '\0';
        strncpy(info->noms[1], sg->noms[1], MAX_NOM - 1);
        info->noms[1][MAX_NOM - 1] = '\0';
        info->winner = sg->winner;
        resp.saved_games_list.count++;
    }
    send_client(resp, client->sock);
}

/* Dispatch central */

void handle_message(ClientPtrVec *clients, Client *client, Message msg)
{
    switch (msg.type) {
        case MSG_CHAT:
            handle_chat(clients, client, msg.content);
            break;

        case MSG_LIST_REQUEST:
            handle_list_request(clients, client);
            break;

        case MSG_SET_BIO:
            handle_set_bio(client, &msg);
            break;

        case MSG_GET_BIO_REQUEST:
            handle_get_bio(clients, client, msg.content);
            break;

        case MSG_ADD_FRIEND:
            handle_add_friend(client, msg.friend_op.friend_name);
            break;

        case MSG_REMOVE_FRIEND:
            handle_remove_friend(client, msg.friend_op.friend_name);
            break;

        case MSG_SET_PRIVATE_MODE:
            handle_private_mode(client, msg.privacy.private_mode);
            break;

        case MSG_SEND_CHALLENGE:
            handle_send_challenge(clients, client, msg.content);
            break;

        case MSG_ANSWER_CHALLENGE:
            handle_answer_challenge(clients, client, &msg);
            break;

        case MSG_PLAY_TURN:
            handle_play_turn(clients, client, msg.trou_selected);
            break;

        case MSG_LIST_GAMES_REQUEST:
            handle_list_games(client);
            break;

        case MSG_OBSERVE_GAME:
            handle_observe_game(clients, client, msg.game_op.game_index);
            break;

        case MSG_STOP_OBSERVE:
            handle_stop_observe(client, msg.game_op.game_index);
            break;

        case MSG_SAVED_GAMES_REQUEST:
            handle_saved_games_request(client);
            break;

        default:
            break;
    }
}
