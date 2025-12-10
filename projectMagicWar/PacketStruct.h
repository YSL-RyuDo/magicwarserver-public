#pragma once
#include <cstdint>

#pragma pack(push, 1)

// ---------------------------
// Signup Request (32 + 64 + 32 = 128)
// ---------------------------
struct PKT_SIGNUP_REQ
{
    char id[32];
    char pw[64];
    char nick[32];
};

// ---------------------------
// Login Request (32 + 64 = 96)
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
    uint16_t result;   // 0=OK, 1=ID exists, 2=Nick exists
};

// ---------------------------
// Login Ack
// ---------------------------
struct PKT_LOGIN_ACK
{
    uint16_t result;  // 0=OK, 1=ID fail, 2=PW fail
    char nick[32];    // 로그인 성공 시 닉네임 포함
};

struct PKT_MATCH_STATE
{
    char msg[32];   // "Wait", "Already", ...
};

struct PKT_MATCH_READY_ACK
{
    char opponent[32];
};
#pragma pack(pop)
