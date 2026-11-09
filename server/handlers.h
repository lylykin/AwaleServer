#ifndef HANDLERS_H
#define HANDLERS_H

/* handlers.h : dispatch des messages entrants. handle_message() est le
 * switch central ; chaque groupe de cas est dans sa propre fonction. */

#include "../shared/types.h"

void handle_message(ClientPtrVec *clients, Client *client, Message msg);

#endif /* HANDLERS_H */
