#pragma once
#include <cstdint>

#pragma pack(push, 1)

// ---------------------------
// Packet Header (8 bytes)
// ---------------------------
struct PACKET_HEADER
{
    uint16_t packetId;
    uint32_t token;
    uint16_t bodySize;
};

// ---------------------------
// Signup Request (128)
// ---------------------------
struct PKT_SIGNUP_REQ
{
    char id[32];
    char pw[64];
    char nick[32];
};

// ---------------------------
// Login Request (96)
// ---------------------------
struct PKT_LOGIN_REQ
{
    char id[32];
    char pw[64];
};

// ---------------------------
// Signup Ack
// ---------------------------
struct PKT_SIGNUP_ACK
{
    uint16_t result;
};

// ---------------------------
// Login Ack
// ---------------------------
struct PKT_LOGIN_ACK_STRUCT
{
    uint16_t result;
    uint32_t token;
    char nick[32];
};

// ---------------------------
// Profile Response
// ---------------------------
struct PKT_RES_PROFILE_DATA
{
    char nickname[32];

    uint32_t level;
    uint32_t exp;

    uint32_t rank;
    uint32_t rankScore;

    char rankName[16];
    uint32_t minScore;
    uint32_t maxScore;
};

struct PKT_CHAT_LOBBY_DATA
{
    char nickname[32];
    char message[128];
};

struct PKT_USER_MAGIC_STRUCT
{
    uint8_t slots[4];
};


struct PKT_RES_USER_MAGIC_STRUCT
{
    int32_t slotCount;     // 예: 4
    int32_t cardIds[4];    // 슬롯 카드 ID
};

struct PKT_REQ_MAGIC_DETAIL_STRUCT
{
    int32_t cardId;
};

struct PKT_RES_MAGIC_DETAIL_STRUCT
{
    int32_t id;
    int32_t attack;
    int32_t cooldown;
    int32_t cost;

    float speed;
    float radius;
    float duration;
    int32_t amount;

    char name[32];
    char effectType[16];
};

struct PKT_REQ_REPLACE_MAGIC_STURCT
{
    int32_t oldCardId;
    int32_t newCardId;
};

struct PKT_RES_REPLACE_USER_MAGIC_STURCT
{
    uint8_t result; // 1 = 성공, 0 = 실패
};

struct PKT_MATCH_RESPONSE_STRUCT
{
    uint8_t result;      // 0=WAIT, 1=OK, 2=FAIL
    uint32_t roomId;

    char userA[32];
    char userB[32];
};

struct PKT_MOVE_STRUCT
{
    char userId[32];
    float x;
    float y;
};

struct PKT_RES_HP_STRUCT
{
    int32_t currentHp;
    int32_t maxHp;
};

struct PKT_HIT_STRUCT
{
    char attackerId[32];
    char targetId[32];
    int32_t cardId;
    int32_t damage;
};

struct PKT_CAST_MAGIC_STRUCT
{
    int cardId;
    float dirX;
    float dirY;
};


struct PKT_SPAWN_MAGIC_STRUCT
{
    char  casterId[32];     // 시전자 ID
    int   cardId;

    float dirX;             // 발사 방향 X
    float dirY;             // 발사 방향 Y

    int   attack;           // 서버 권위 공격력
    float projectileSpeed;  // 서버 권위 속도
};

struct PKT_REQ_MAGIC_DETAIL_STRUCT_GAME
{
    int cardId;
};

struct PKT_RES_MAGIC_DETAIL_STRUCT_GAME
{
    int cardId;

    int attack;
    float speed;
    float cooldown;

    float radius;
    float duration;
    int amount;
};

struct PKT_DIE_STRUCT
{
    char deadUserId[32];
};

struct PKT_RESULT_STRUCT
{
    char userId[32];
};


//효과
struct PKT_STATUS_SLOW_STRUCT
{
    char targetId[32];
    float slowRatio;     // 예: 0.5f = 이동속도 50%
    int durationMs;      // 지속 시간 (ms)
};

struct PKT_STATUS_STUN_STRUCT
{
    char targetId[32];
    int durationMs;      // 지속 시간 (ms)
};
#pragma pack(pop)
