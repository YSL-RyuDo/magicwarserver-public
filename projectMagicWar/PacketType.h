#pragma once

enum PacketType {
    Signup = 0,
    Login = 1,
    SignupSuccess = 2,
    SignupNickError = 3,
    SignupIDError = 4,
    LoginSuccess = 5,
    LoginIDError = 6,
    LoginPWError = 7
};
