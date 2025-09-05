#pragma once

enum PacketType {
    Signup = 0,
    Login = 1,
    SignupSuccess = 2,
    SignupNickError = 3,
    SignupIDError = 4,
    LoginSuccess = 5,
    LoginIDError = 6,
    LoginPWError = 7,
    RequestProfile = 8,
    ResponseProfile = 9,
    RequestUserMagic = 10,   // 유저 마법 슬롯 요청
    ResponseUserMagic = 11,   // 유저 마법 슬롯 응답
    RequestSetUserMagic = 12,    // 슬롯 변경 요청 (클라 → 서버)
    ResponseSetUserMagic = 13,
    RequestReplaceUserMagic = 14,
	ResponseReplaceUserMagic = 15
};
