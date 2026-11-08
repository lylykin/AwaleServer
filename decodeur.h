#pragma once

#define BUF_SIZE 4096
#define MAX_NOM 32

// Message types
typedef enum {
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

// Message structure
typedef struct {
    MsgType type;
    union {
        char content[BUF_SIZE];
        int trou_selected;
        struct {
            int accepted;
            char challenger_name[MAX_NOM];
            char challenged_name[MAX_NOM];
            int side;
        } challenge;
        struct {
            char player_name[MAX_NOM];
            int score;
        } game_info;
        struct {
            int board[2][6];
            int grenier[2];
            int current_player;
        } board;
        struct {
            char bio_lines[10][256];
            int line_count;
        } bio;
        struct {
            char friend_name[MAX_NOM];
        } friend_op;
        struct {
            int private_mode;
        } privacy;
        struct {
            int game_index;
            char player1[MAX_NOM];
            char player2[MAX_NOM];
        } game_info_msg;
        
    };
} Message;

// Function declarations using int for socket (platform independent)
int send_server(Message msg, int socket);
int send_client(Message msg, int sock);
Message msg_from_server(int socket);
void handle_msg_server(int sock, Message msg);
void set_player_side(int side);
