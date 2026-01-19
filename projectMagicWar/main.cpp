#include <iostream>
#include "IOCP_GameServer.h"
using namespace std;

int main()
{
    IOCP_GameServer server(9000, 0, 8);

    server.Start();

    while (true)
    {
        char c;
        cin >> c;
        if (c == 'q') break;
    }
    
    server.Stop();
}
