#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#define MAX_PAYLOAD 256

typedef enum {
    MSG_LOGIN = 1,
    MSG_LOGIN_SUCCESS,
    MSG_LOGIN_ALREADY_LOGGED_IN,
    MSG_LOGIN_FAILURE,
    MSG_REGISTER,
    MSG_REGISTER_SUCCESS,
    MSG_REGISTER_LOGIN_TAKEN,
    MSG_REGISTER_FAILURE,
    MSG_JOIN_LOBBY,
    MSG_JOIN_LOBBY_SUCCESS,
    MSG_JOIN_LOBBY_FAIL,
    MSG_LOBBY_READY,
    MSG_ALL_LOBBIES_FULL,
    MSG_GAME_START,
    MSG_MOVE,
    MSG_RESULT,
    MSG_WRONG_MOVE,
    MSG_RANKING,
    MSG_RANKING_RESPONSE,
    MSG_ACTIVE_USERS,
    MSG_ACTIVE_USERS_RESPONSE,
    MSG_HISTORY,
    MSG_ERROR,
    MSG_SERVER_DISCOVERY,
    MSG_SERVER_DISCOVERY_RESPONSE,
} MessageType;

typedef struct {
    MessageType type;
    uint16_t length;
    char value[MAX_PAYLOAD];
} __attribute__((packed)) TLVMessage;

typedef struct {
    int id;
    char login[32];
} UserInfo;

#endif