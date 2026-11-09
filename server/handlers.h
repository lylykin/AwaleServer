#pragma once

// handlers.h : dispatch des messages entrants.

#include "../shared/types.h"

void handle_message(ClientPtrVec *clients, Client *client, Message msg);
