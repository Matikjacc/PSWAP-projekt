#include <stdint.h>
#define MAX_PAYLOAD 256

typedef enum {
    MSG_LOGIN = 1,
    MSG_LOGIN_SUCCESS,
    MSG_LOGIN_FAILURE,
    MSG_REGISTER,
    MSG_REGISTER_SUCCESS,
    MSG_REGISTER_FAILURE,
    MSG_JOIN_LOBBY,
    MSG_JOIN_LOBBY_SUCCESS,
    MSG_LOBBY_READY,
    MSG_ALL_LOBBIES_FULL,
    MSG_GAME_START,
    MSG_MOVE,
    MSG_RESULT,
    MSG_RANKING,
    MSG_RANKING_RESPONSE,
    MSG_HISTORY,
    MSG_ERROR
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
