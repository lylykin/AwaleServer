#ifndef CLIENT_H
#define CLIENT_H

/* ------------------------------------------------------------------ */
/*  client.h - top-level header for the client.                       */
/* ------------------------------------------------------------------ */

#include "../shared/platform.h"

#define CRLF     "\r\n"
#define PORT     1977

#define BUF_SIZE 4096
#define MAX_NOM  32   /* same size as server side (game.h) */

static void init(void);
static void end(void);
static void app(const char *address, const char *name);
static int init_connection(const char *address);
static void end_connection(int sock);
#endif /* guard */
