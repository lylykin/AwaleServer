#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "client.h"
static char challenger[BUF_SIZE] = "";
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

static void app(const char *address, const char *name)
{
   SOCKET sock = init_connection(address);
   char buffer[BUF_SIZE];

   fd_set rdfs;

   /* send our name */
   write_server(sock, name);

   while(1)
   {
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the socket */
      FD_SET(sock, &rdfs);

      if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         fgets(buffer, BUF_SIZE - 1, stdin);
         {
            char *p = NULL;
            p = strstr(buffer, "\n");
            if(p != NULL)
            {
               *p = 0;
            }
            else
            {
               /* fclean */
               buffer[BUF_SIZE - 1] = 0;
            }
         }
         if (!repondre_challenge(sock, buffer)){
                     if (strncmp(buffer, "/challenge", strlen("/challenge")) == 0){
                         challenge_autre_client(sock, buffer);
                     }
                     else{
                         write_server(sock, buffer);
                     }
                 }
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         int n = read_server(sock, buffer);
         /* server down */
         if(n == 0)
         {
            printf("Server disconnected !\n");
            break;
         }
         if (strncmp(buffer, "CHALLENGE_FROM ", strlen("CHALLENGE_FROM ")) == 0)
         {
            snprintf(challenger, sizeof challenger, "%s", buffer + strlen("CHALLENGE_FROM "));
            printf("%s vous défie ! Accepter ? (o/n)\n", challenger);
         }
         else
         {
            puts(buffer);
         }
      }
   }

   end_connection(sock);
}

static int init_connection(const char *address)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };
   struct hostent *hostinfo;

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   hostinfo = gethostbyname(address);
   if (hostinfo == NULL)
   {
      fprintf (stderr, "Unknown host %s.\n", address);
      exit(EXIT_FAILURE);
   }

   sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
   {
      perror("connect()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_server(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      exit(errno);
   }

   buffer[n] = 0;

   return n;
}
static void challenge_autre_client(SOCKET sock, char *buffer){
     char msg[BUF_SIZE];
     const char *adversaire = buffer + strlen("/challenge ");
     if (*adversaire == '\0'){
         printf("Usage: /challenge <pseudo>\n");
         return;
     }
     snprintf(msg, sizeof msg, "CHALLENGE %s", adversaire);
     write_server(sock, msg);
}
static  int repondre_challenge(SOCKET sock, char *buffer){
        char msg[BUF_SIZE];
        if (challenger[0] == '\0'){
            return 0;
        }
        else if (strcmp(buffer, "o") == 0){
            snprintf(msg, sizeof msg, "ACCEPT %s", challenger);
        }
        else if (strcmp(buffer,"n") == 0){
            snprintf(msg, sizeof msg, "REFUSE %s", challenger);
        }
        else {
            return 0;
        }
        write_server(sock, msg);
        challenger[0] = '\0';
        return 1;
}
static void write_server(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   if(argc < 2)
   {
      printf("Usage : %s [address] [pseudo]\n", argv[0]);
      return EXIT_FAILURE;
   }

   init();

   app(argv[1], argv[2]);

   end();

   return EXIT_SUCCESS;
}
