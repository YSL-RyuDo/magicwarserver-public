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
struct PKT_LOGIN_ACK
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

// ---------------------------
// Match
// ---------------------------
struct PKT_MATCH_STATE
{
    char msg[32];
};

struct PKT_MATCH_READY_ACK
{
    char opponent[32];
};

#pragma pack(pop)
