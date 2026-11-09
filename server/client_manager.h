#pragma once

/*client_manager.h : gestion desclients connectés (recherche,
 déconnexion, broadcast)  */

#include "../shared/types.h"

extern PlayerPtrVec players;

Client *find_client(ClientPtrVec *clients, const char *name);

void remove_client(ClientPtrVec *clients, Client *to_remove);

void clear_clients(ClientPtrVec *clients);

void send_message_to_all_clients(ClientPtrVec *clients, Client *sender, const char *buffer, char from_server);

void send_list(ClientPtrVec *clients, SOCKET to);
