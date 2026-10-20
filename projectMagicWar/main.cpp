#include <iostream>
#include <cstdlib>
#include "IOCP_GameServer.h"
using namespace std;

int main()
{
    char* portText = nullptr;
    size_t portSize = 0;
    _dupenv_s(&portText, &portSize, "ARCANA_PORT");
    const int port = portText ? std::atoi(portText) : 9000;
    std::free(portText);
    if (port < 1 || port > 65535) return 1;
    IOCP_GameServer server(static_cast<unsigned short>(port), 0, 8);

    if (!server.Start()) return 1;

    while (true)
    {
        char c;
        if (!(cin >> c) || c == 'q') break;
    }
    
    server.Stop();
}
