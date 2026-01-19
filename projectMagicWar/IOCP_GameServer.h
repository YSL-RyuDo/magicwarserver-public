#pragma once
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <vector>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>

#include "Session.h"
#include "PacketManager.h"
#include "MatchManager.h"
#include "BufferPool.h"
#include "OverlappedPool.h"
#include "SessionPool.h"
#include "IOCP_Send.h"
#include "IOCP_Recv.h"
#include "UserManager.h"
#include "DBWorker.h"
#include "DBWorker.h"

class IOCP_GameServer
{
public:
    IOCP_GameServer(unsigned short port, int workerCount = 0, int acceptCount = 16);
    ~IOCP_GameServer();

    bool Start();
    void Stop();

private:
    void LoadExtensionFunctions();
    void PostAccept();

    void WorkerThread();
    void AcceptCompletion(OverlappedEx* ov);
    void RecvCompletion(Session* session, OverlappedEx* ov, DWORD bytes);
    void SendCompletion(Session* session, OverlappedEx* ov);

    void CloseSession(Session* session);

private:
    SOCKET listenSock;
    HANDLE iocpHandle;

    LPFN_ACCEPTEX fnAcceptEx = nullptr;
    LPFN_GETACCEPTEXSOCKADDRS fnGetAcceptExSockAddr = nullptr;

    std::vector<std::thread> workerThreads;
    std::atomic<bool> running;

    unsigned short port;
    int workerCount;
    int acceptCount;

    std::unordered_map<uint64_t, Session*> sessions;
    std::mutex sessionMtx;
    std::atomic<uint64_t> sessionIdGen;

    MatchManager matchMgr;
    std::unordered_map<std::string, Session*> userMap;

    BufferPool bufPool;
    OverlappedPool ovPool;
    SessionPool sessionPool;

    IOCP_Recv* recvProcessor;
    IOCP_Send* sendProcessor;

    DBWorker dbWorker;
};
