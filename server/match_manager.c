#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "match_manager.h"
#include "client_manager.h"
#include "persistence.h"

MatchVec matches;
SavedGameVec saved_games;

void save_game(Match *m, int winner)
{
    SavedGame sg;
    strncpy(sg.noms[0], m->noms[0], MAX_NOM - 1);
    sg.noms[0][MAX_NOM - 1] = '\0';
    strncpy(sg.noms[1], m->noms[1], MAX_NOM - 1);
    sg.noms[1][MAX_NOM - 1] = '\0';
    memcpy(&sg.g, &m->g, sizeof(Game));
    sg.winner = winner;
    vec_push(&saved_games, sg);
}

/* Rattache un client reconnecté à sa partie en cours. */
void resume_match(ClientPtrVec *clients, Client *c)
{
    if (!c->player) return;

    for (int k = 0; k < matches.length; k++) {
        Match *m = &matches.data[k];
        if (!m->actif) continue;

        int side = -1;
        if (strcmp(m->noms[0], c->player->name) == 0) side = m->side_of[0];
        else if (strcmp(m->noms[1], c->player->name) == 0) side = m->side_of[1];
        if (side < 0) continue;

        c->player->match = k;
        c->player->side = side;

        const char *opp_name = (strcmp(m->noms[0], c->player->name) == 0)
                             ? m->noms[1] : m->noms[0];
        Message msg;

        msg.type = MSG_GAME_START;
        strncpy(msg.challenge.challenger_name, opp_name, MAX_NOM - 1);
        msg.challenge.challenger_name[MAX_NOM - 1] = '\0';
        strncpy(msg.challenge.challenged_name, c->player->name, MAX_NOM - 1);
        msg.challenge.challenged_name[MAX_NOM - 1] = '\0';
        msg.challenge.side = side;
        send_client(msg, c->sock);

        msg.type = MSG_BOARD_UPDATE;
        memcpy(msg.board.board, m->g.plateau, sizeof(m->g.plateau));
        memcpy(msg.board.grenier, m->g.grenier, sizeof(m->g.grenier));
        msg.board.current_player = m->g.joueur;
        send_client(msg, c->sock);

        msg.type = (m->g.joueur == side) ? MSG_YOUR_TURN : MSG_WAIT;
        send_client(msg, c->sock);

        Client *opp = find_client(clients, opp_name);
        if (opp) {
            msg.type = MSG_CHAT;
            snprintf(msg.content, BUF_SIZE - 1,
                     "%s s'est reconnecte, la partie reprend." CRLF, c->player->name);
            send_client(msg, opp->sock);
        }
        return;
    }
}

void start_match(ClientPtrVec *clients, Client *adv, Client *me)
{
    (void)clients;
    if (!me->player || !adv->player) return;
    Message msg;

    if (me->player->match >= 0 || adv->player->match >= 0) {
        msg.type = MSG_CHAT;
        strncpy(msg.content, "Un des joueurs est deja en partie." CRLF, BUF_SIZE - 1);
        msg.content[BUF_SIZE - 1] = '\0';
        send_client(msg, me->sock);
        me->pending[0] = '\0';
        return;
    }

    int k;
    for (k = 0; k < matches.length; k++) {
        if (!matches.data[k].actif) break;
    }
    if (k == matches.length) {
        /* Aucun slot libre : pousse une nouvelle partie dans le vec. */
        Match new_match;
        memset(&new_match, 0, sizeof(Match));
        vec_init(&new_match.observers);
        vec_push(&matches, new_match);
    }

    Match *m = &matches.data[k];
    m->actif = 1;
    vec_init(&m->observers);

    int starter = rand() % 2;
    init_game(&m->g, starter);

    strncpy(m->noms[0], adv->player->name, MAX_NOM - 1);
    m->noms[0][MAX_NOM - 1] = '\0';
    strncpy(m->noms[1], me->player->name, MAX_NOM - 1);
    m->noms[1][MAX_NOM - 1] = '\0';

    adv->player->match = k;
    me->player->match = k;
    if (starter == 0) {
        adv->player->side = 0;
        me->player->side = 1;
    } else {
        adv->player->side = 1;
        me->player->side = 0;
    }

    m->side_of[0] = adv->player->side;
    m->side_of[1] = me->player->side;

    save_persistence_data(&players, &matches);

    me->pending[0] = '\0';
    adv->pending[0] = '\0';

    msg.type = MSG_GAME_START;
    strncpy(msg.challenge.challenger_name, me->player->name, MAX_NOM - 1);
    msg.challenge.challenger_name[MAX_NOM - 1] = '\0';
    strncpy(msg.challenge.challenged_name, adv->player->name, MAX_NOM - 1);
    msg.challenge.challenged_name[MAX_NOM - 1] = '\0';
    msg.challenge.side = adv->player->side;
    send_client(msg, adv->sock);

    msg.type = MSG_GAME_START;
    strncpy(msg.challenge.challenger_name, me->player->name, MAX_NOM - 1);
    msg.challenge.challenger_name[MAX_NOM - 1] = '\0';
    strncpy(msg.challenge.challenged_name, adv->player->name, MAX_NOM - 1);
    msg.challenge.challenged_name[MAX_NOM - 1] = '\0';
    msg.challenge.side = me->player->side;
    send_client(msg, me->sock);

    msg.type = (m->g.joueur == adv->player->side) ? MSG_YOUR_TURN : MSG_WAIT;
    send_client(msg, adv->sock);

    msg.type = (m->g.joueur == me->player->side) ? MSG_YOUR_TURN : MSG_WAIT;
    send_client(msg, me->sock);

    msg.type = MSG_BOARD_UPDATE;
    memcpy(msg.board.board, m->g.plateau, sizeof(m->g.plateau));
    memcpy(msg.board.grenier, m->g.grenier, sizeof(m->g.grenier));
    msg.board.current_player = m->g.joueur;
    send_client(msg, adv->sock);
    send_client(msg, me->sock);

    msg.type = MSG_CHAT;
    snprintf(msg.content, BUF_SIZE - 1,
             "Vous etes le joueur %d. Partie demarree contre %s." CRLF,
             adv->player->side, me->player->name);
    send_client(msg, adv->sock);
    snprintf(msg.content, BUF_SIZE - 1,
             "Vous etes le joueur %d. Partie demarree contre %s." CRLF,
             me->player->side, adv->player->name);
    send_client(msg, me->sock);
}

void handle_move(ClientPtrVec *clients, Client *me, int trou)
{
    if (!me->player || me->player->match < 0) return;
    Match *m = &matches.data[me->player->match];
    Message msg;

    /* Trouve l'adversaire. */
    Client *opp = NULL;
    {
        for (int ci = 0; ci < clients->length; ci++) {
            Client *c = clients->data[ci];
            if (c->active && c != me && c->player &&
                c->player->match == me->player->match) {
                opp = c;
                break;
            }
        }
    }

    if (me->player->side != m->g.joueur) {
       msg.type = MSG_NOT_YOUR_TURN;
       send_client(msg, me->sock);
       return;
    }

    if (play_move(&m->g, trou) < 0) {
       msg.type = MSG_MOVE_INVALID;
       send_client(msg, me->sock);
       return;
    }

    msg.type = MSG_BOARD_UPDATE;
    memcpy(msg.board.board, m->g.plateau, sizeof(m->g.plateau));
    memcpy(msg.board.grenier, m->g.grenier, sizeof(m->g.grenier));
    msg.board.current_player = m->g.joueur;
    send_client(msg, me->sock);
    if (opp) send_client(msg, opp->sock);

    for (int oi = 0; oi < m->observers.length; oi++) {
        Client *obs = m->observers.data[oi];
        if (obs->active && obs != me && obs != opp) {
            send_client(msg, obs->sock);
        }
    }

    if (!end_game(m->g.plateau)) {
       int cur_side = m->g.joueur;
       Client *cur_client = (cur_side == me->player->side) ? me : opp;
       Client *other_client = (cur_side == me->player->side) ? opp : me;

       if (cur_client) {
           msg.type = MSG_YOUR_TURN;
           send_client(msg, cur_client->sock);
       }
       if (other_client) {
           msg.type = MSG_WAIT;
           send_client(msg, other_client->sock);
       }

       msg.type = MSG_CHAT;
       /* m->g.joueur est un side (0/1) ; trouver le nom via side_of[] */
       const char *next_name = (m->side_of[0] == m->g.joueur) ? m->noms[0] : m->noms[1];
       snprintf(msg.content, BUF_SIZE - 1,
                "%s a joue le trou %d. C'est au tour de %s." CRLF,
                me->player->name, trou, next_name);
       //send_client(msg, me->sock);
       //if (opp) send_client(msg, opp->sock);

       for (int oi = 0; oi < m->observers.length; oi++) {
           Client *obs = m->observers.data[oi];
           if (obs->active) send_client(msg, obs->sock);
       }
    }

    if (end_game(m->g.plateau)) {
       int g0 = m->g.grenier[0], g1 = m->g.grenier[1];
       int winner = -1;
       if (g0 > g1) winner = 0;
       else if (g1 > g0) winner = 1;

       msg.type = MSG_GAME_OVER;
       msg.game_over.winner = winner;
       msg.game_over.score0 = g0;
       msg.game_over.score1 = g1;
       send_client(msg, me->sock);
       if (opp) send_client(msg, opp->sock);

       for (int oi = 0; oi < m->observers.length; oi++) {
           Client *obs = m->observers.data[oi];
           if (obs->active) send_client(msg, obs->sock);
       }

       save_game(m, winner);

       m->actif = 0;
       me->player->match = -1;
       if (opp && opp->player) opp->player->match = -1;
       vec_clear(&m->observers);

       save_persistence_data(&players, &matches);
    } else {
       save_persistence_data(&players, &matches);
    }
}
