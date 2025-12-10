#include "IOCP_GameServer.h"
#include <iostream>

IOCP_GameServer::IOCP_GameServer(unsigned short port, int workerCount, int acceptCount)
    : port(port),
    workerCount(workerCount ? workerCount : std::thread::hardware_concurrency()),
    acceptCount(acceptCount),
    listenSock(INVALID_SOCKET),
    iocpHandle(nullptr),
    running(false),
    sessionIdGen(1),
    sessionPool(2000)
{
    recvProcessor = nullptr;
    sendProcessor = nullptr;
}

IOCP_GameServer::~IOCP_GameServer()
{
    Stop();
}

bool IOCP_GameServer::Start()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    listenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(listenSock, (sockaddr*)&addr, sizeof(addr));
    listen(listenSock, SOMAXCONN);

    iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    CreateIoCompletionPort((HANDLE)listenSock, iocpHandle, 0, 0);

    LoadExtensionFunctions();

    recvProcessor = new IOCP_Recv(&bufPool, &ovPool);
    sendProcessor = new IOCP_Send(&ovPool, &bufPool);

    PacketManager::Init(&userMap, &matchMgr, sendProcessor);

    running = true;

    for (int i = 0; i < workerCount; i++)
        workerThreads.emplace_back(&IOCP_GameServer::WorkerThread, this);

    for (int i = 0; i < acceptCount; i++)
        PostAccept();

    std::cout << "IOCP Server Started. port = " << port << "\n";
    return true;
}

void IOCP_GameServer::Stop()
{
    if (!running) return;
    running = false;

    closesocket(listenSock);

    for (int i = 0; i < workerCount; i++)
        PostQueuedCompletionStatus(iocpHandle, 0, 0, NULL);

    for (auto& t : workerThreads)
        if (t.joinable()) t.join();

    CloseHandle(iocpHandle);

    delete recvProcessor;
    delete sendProcessor;

    for (auto& kv : sessions)
        sessionPool.Release(kv.second);

    sessions.clear();

    WSACleanup();
}

void IOCP_GameServer::LoadExtensionFunctions()
{
    DWORD bytes = 0;

    GUID guidAccept = WSAID_ACCEPTEX;
    WSAIoctl(listenSock, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidAccept, sizeof(guidAccept),
        &fnAcceptEx, sizeof(fnAcceptEx),
        &bytes, nullptr, nullptr);

    GUID guidAddr = WSAID_GETACCEPTEXSOCKADDRS;
    WSAIoctl(listenSock, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidAddr, sizeof(guidAddr),
        &fnGetAcceptExSockAddr, sizeof(fnGetAcceptExSockAddr),
        &bytes, nullptr, nullptr);
}

void IOCP_GameServer::PostAccept()
{
    OverlappedEx* ov = ovPool.Acquire();
    ZeroMemory(ov, sizeof(OverlappedEx));

    ov->op = OverlappedEx::OP_ACCEPT;
    ov->bufferPtr = bufPool.Acquire();
    ov->wsaBuf.buf = ov->bufferPtr;
    ov->wsaBuf.len = IOCP_BUFFER_SIZE;

    ov->acceptSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    DWORD bytes = 0;

    BOOL ret = fnAcceptEx(
        listenSock,
        ov->acceptSock,
        ov->bufferPtr,
        0,
        sizeof(sockaddr_in) + 16,
        sizeof(sockaddr_in) + 16,
        &bytes,
        ov
    );

    if (!ret && WSAGetLastError() != WSA_IO_PENDING)
    {
        closesocket(ov->acceptSock);
        bufPool.Release(ov->bufferPtr);
        ovPool.Release(ov);
        PostAccept();
    }
}

void IOCP_GameServer::WorkerThread()
{
    while (running)
    {
        DWORD bytes = 0;
        ULONG_PTR key = 0;
        LPOVERLAPPED ov = nullptr;

        BOOL ok = GetQueuedCompletionStatus(iocpHandle, &bytes, &key, &ov, INFINITE);

        if (!running) break;
        if (!ov) continue;

        OverlappedEx* ex = (OverlappedEx*)ov;

        switch (ex->op)
        {
        case OverlappedEx::OP_ACCEPT:
            AcceptCompletion(ex);
            break;

        case OverlappedEx::OP_RECV:
            RecvCompletion((Session*)ex->session, ex, bytes);
            break;

        case OverlappedEx::OP_SEND:
            SendCompletion((Session*)ex->session, ex);
            break;
        }
    }
}

void IOCP_GameServer::AcceptCompletion(OverlappedEx* ov)
{
    setsockopt(ov->acceptSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
        (char*)&listenSock, sizeof(listenSock));

    Session* session = sessionPool.Acquire();
    session->sock = ov->acceptSock;
    session->sessionId = sessionIdGen++;

    {
        std::lock_guard<std::mutex> lock(sessionMtx);
        sessions[session->sessionId] = session;
    }

    CreateIoCompletionPort((HANDLE)session->sock, iocpHandle, (ULONG_PTR)session, 0);

    recvProcessor->PostRecv(session);

    bufPool.Release(ov->bufferPtr);
    ovPool.Release(ov);

    PostAccept();
}

void IOCP_GameServer::RecvCompletion(Session* session, OverlappedEx* ov, DWORD bytes)
{
    recvProcessor->RecvCompletion(session, ov, bytes);
}

void IOCP_GameServer::SendCompletion(Session* session, OverlappedEx* ov)
{
    sendProcessor->SendCompletion(session, ov);
}

void IOCP_GameServer::CloseSession(Session* session)
{
    if (session->closing.exchange(true))
        return;

    closesocket(session->sock);

    {
        std::lock_guard<std::mutex> lock(sessionMtx);
        sessions.erase(session->sessionId);
    }

    sessionPool.Release(session);
}
