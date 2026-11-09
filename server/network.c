#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "network.h"
#include "../shared/types.h"

/* Crée le socket d'écoute sur PORT. SO_REUSEADDR permet de relancer
 * le serveur immédiatement après un crash sans attendre le OS. */
int init_connection(void)
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

void end_connection(int sock)
{
   closesocket(sock);
}
