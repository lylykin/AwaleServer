#ifndef MATCH_MANAGER_H
#define MATCH_MANAGER_H

/* match_manager.h : cycle de vie des parties (démarrage, reprise, coups, fin).
 * matches est un vec dynamique indexé ; saved_games est un vec dynamique. */

#include "../shared/types.h"

extern MatchVec matches;
extern SavedGameVec saved_games;

void resume_match(ClientPtrVec *clients, Client *c);
void start_match(ClientPtrVec *clients, Client *adv, Client *me);
void handle_move(ClientPtrVec *clients, Client *me, int trou);
void save_game(Match *m, int winner);

#endif /* MATCH_MANAGER_H */
