#ifndef NETWORK_H
#define NETWORK_H

/* network.h : création et fermeture du socket d'écoute TCP */

#include "../shared/platform.h"

int  init_connection(void);   /* bind + listen sur PORT */
void end_connection(int sock); /* ferme le socket */

#endif /* NETWORK_H */
