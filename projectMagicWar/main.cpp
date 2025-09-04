#include "GameServer.h"
#include <iostream>

int main() {
    try {
        GameServer server(7777);
        server.start();
    }
    catch (const std::exception& e) {
        std::cerr << "서버 오류: " << e.what() << std::endl;
    }
    return 0;
}
