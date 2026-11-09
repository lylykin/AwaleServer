#pragma once

#define BUF_SIZE 4096
#define MAX_NOM 32
#define MAX_LIST_NAMES  100   /* max names in a list response          */
#define MAX_GAMES_IN_MSG 50   /* max games in a list response          */
#define MAX_SAVED_IN_MSG 10   /* max saved games sent in one message   */

// Message types
typedef enum {
    MSG_ERROR = -1,
    MSG_PLAY_TURN,
    MSG_CONNECT,
    MSG_SEND_CHALLENGE,
    MSG_ANSWER_CHALLENGE,
    MSG_CHAT,
    MSG_LIST_REQUEST,
    MSG_LIST_RESPONSE,
    MSG_GAME_START,
    MSG_BOARD_UPDATE,
    MSG_GAME_OVER,
    MSG_YOUR_TURN,
    MSG_WAIT,
    MSG_MOVE_INVALID,
    MSG_NOT_YOUR_TURN,
    MSG_SET_BIO,
    MSG_GET_BIO_REQUEST,
    MSG_GET_BIO_RESPONSE,
    MSG_ADD_FRIEND,
    MSG_REMOVE_FRIEND,
    MSG_SET_PRIVATE_MODE,
    MSG_LIST_GAMES_REQUEST,
    MSG_LIST_GAMES_RESPONSE,
    MSG_OBSERVE_GAME,
    MSG_STOP_OBSERVE,
    MSG_SAVED_GAMES_REQUEST,
    MSG_SAVED_GAMES_RESPONSE,
    MSG_LOAD_GAME_REQUEST,
    MSG_LOAD_GAME_RESPONSE
} MsgType;

/* A single saved-game entry as sent over the wire (names + winner only,
 * not the full board - the client only needs the summary). */
typedef struct {
    char noms[2][MAX_NOM];
    int winner; /* -1: draw, 0 or 1: index of winner */
} SavedGameInfo;

/* A single match summary as sent over the wire. */
typedef struct {
    char player1[MAX_NOM];
    char player2[MAX_NOM];
} GameInfoMsg;

// Message structure - content[] is only for MSG_CHAT (free text).
// All other message types use their dedicated union field.
typedef struct {
    MsgType type;
    union {
        char content[BUF_SIZE];                    /* MSG_CHAT only            */
        int trou_selected;                          /* MSG_PLAY_TURN            */
        struct {                                    /* challenges + game start  */
            int accepted;
            char challenger_name[MAX_NOM];
            char challenged_name[MAX_NOM];
            int side;
        } challenge;
        struct {                                    /* MSG_BOARD_UPDATE         */
            int board[2][6];
            int grenier[2];
            int current_player;
        } board;
        struct {                                    /* MSG_SET_BIO / GET_BIO_RESPONSE */
            char bio_lines[10][256];
            int line_count;
        } bio;
        struct {                                    /* MSG_ADD/REMOVE_FRIEND    */
            char friend_name[MAX_NOM];
        } friend_op;
        struct {                                    /* MSG_SET_PRIVATE_MODE     */
            int private_mode;
        } privacy;
        struct {                                    /* MSG_OBSERVE_GAME / STOP  */
            int game_index;
        } game_op;
        struct {                                    /* MSG_LIST_RESPONSE        */
            int count;
            char names[MAX_LIST_NAMES][MAX_NOM];
        } player_list;
        struct {                                    /* MSG_LIST_GAMES_RESPONSE  */
            int count;
            GameInfoMsg games[MAX_GAMES_IN_MSG];
        } game_list;
        struct {                                    /* MSG_SAVED_GAMES_RESPONSE */
            int count;
            SavedGameInfo games[MAX_SAVED_IN_MSG];
        } saved_games_list;
        struct {                                    /* MSG_GAME_OVER            */
            int winner;                             /* -1: draw, 0/1: winner    */
            int score0;
            int score1;
        } game_over;
    };
} Message;

// Function declarations using int for socket (platform independent)
int send_server(Message msg, int socket);
int send_client(Message msg, int sock);
Message msg_from_server(int socket);
void handle_msg_server(int sock, Message msg);
void set_player_side(int side);
void set_game_names(const char *my_name, int my_side, const char *opp_name);
