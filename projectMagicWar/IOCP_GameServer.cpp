#include "IOCP_GameServer.h"
#include "PacketManager.h"
#include "DBManager.h"
#include "DBWorkerInstance.h"

#include <iostream>
#include <ws2tcpip.h> // InetNtopA 사용 위해 필요

// IOCP 기반 TCP 게임 서버 클래스 구현
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

// 서버 종료 시 리소스 정리
IOCP_GameServer::~IOCP_GameServer()
{
    Stop();
}

// 서버 소켓 초기화, IOCP 연결, 워커 스레드 및 DB 초기화
bool IOCP_GameServer::Start()
{
    WSADATA wsa;

    int ret = WSAStartup(MAKEWORD(2, 2), &wsa);

    listenSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    ret = ::bind(listenSock, (sockaddr*)&addr, sizeof(addr));

    ret = ::listen(listenSock, SOMAXCONN);

    iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    CreateIoCompletionPort((HANDLE)listenSock, iocpHandle, 0, 0);

    LoadExtensionFunctions();

    recvProcessor = new IOCP_Recv(&bufPool, &ovPool);
    sendProcessor = new IOCP_Send(&ovPool, &bufPool);

    PacketManager::Init(&userMap, &matchMgr, sendProcessor);

    running = true;

    for (int i = 0; i < workerCount; i++)
    {
        workerThreads.emplace_back(&IOCP_GameServer::WorkerThread, this);
    }

    for (int i = 0; i < acceptCount; i++)
        PostAccept();

    DBManager::Init("127.0.0.1", "SET_DB_USER", "SET_DB_PASSWORD", "SET_DB_NAME", 3307);
    dbWorker.Start();
    g_dbWorker = &dbWorker;

    UserManager::LoadRankTable();
    UserManager::LoadCardTable();
    std::cout << "[SERVER] IOCP Game Server started. port=" << port
        << " workers=" << workerCount
        << " accepts=" << acceptCount
        << std::endl;
    return true;
}

// 서버 종료, 스레드 종료 및 리소스 해제
void IOCP_GameServer::Stop()
{
    if (!running) return;
    running = false;

    closesocket(listenSock);

    for (int i = 0; i < workerCount; i++)
        PostQueuedCompletionStatus(iocpHandle, 0, 0, NULL);

    for (auto& t : workerThreads)
        if (t.joinable()) t.join();

    if (g_dbWorker)
    {
        g_dbWorker->Stop();
        g_dbWorker = nullptr;
    }

    CloseHandle(iocpHandle);

    delete recvProcessor;
    delete sendProcessor;

    for (auto& kv : sessions)
        sessionPool.Release(kv.second);

    sessions.clear();
    WSACleanup();
}

// AcceptEx 및 GetAcceptExSockAddrs 함수 포인터 로드
void IOCP_GameServer::LoadExtensionFunctions()
{
    DWORD bytes = 0;

    GUID guidAcceptEx = WSAID_ACCEPTEX;
    WSAIoctl(listenSock,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidAcceptEx,
        sizeof(guidAcceptEx),
        &fnAcceptEx,
        sizeof(fnAcceptEx),
        &bytes,
        nullptr,
        nullptr);

    GUID guidGetAddr = WSAID_GETACCEPTEXSOCKADDRS;
    WSAIoctl(listenSock,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidGetAddr,
        sizeof(guidGetAddr),
        &fnGetAcceptExSockAddr,
        sizeof(fnGetAcceptExSockAddr),
        &bytes,
        nullptr,
        nullptr);

}

// 새 클라이언트 연결 요청을 수신하기 위해 비동기 Accept 작업 게시
void IOCP_GameServer::PostAccept()
{
    OverlappedEx* ov = ovPool.Acquire();
    ZeroMemory(static_cast<OVERLAPPED*>(ov), sizeof(OVERLAPPED));
    ov->op = OverlappedEx::OP_ACCEPT;

    constexpr DWORD addrLen = sizeof(sockaddr_in) + 16;

    ov->bufferPtr = bufPool.Acquire();

    ov->acceptSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

    DWORD bytes = 0;

    BOOL ret = fnAcceptEx(listenSock, ov->acceptSock, ov->bufferPtr, 0, addrLen, addrLen, &bytes, static_cast<LPOVERLAPPED>(ov));

    if (!ret)
    {
        int err = WSAGetLastError();

        if (err != WSA_IO_PENDING)
        {
            std::cout << "[ACCEPT][치명적] AcceptEx 즉시 실패\n";
            closesocket(ov->acceptSock);
            bufPool.Release(ov->bufferPtr);
            ovPool.Release(ov);
        }
    }
}

// IOCP 워커 스레드 루프
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

        Session* session = reinterpret_cast<Session*>(key);
        OverlappedEx* ex = reinterpret_cast<OverlappedEx*>(ov);

        switch (ex->op)
        {
        case OverlappedEx::OP_ACCEPT:
            AcceptCompletion(ex);
            break;

        case OverlappedEx::OP_RECV:
            RecvCompletion(reinterpret_cast<Session*>(key), ex, bytes);
            break;

        case OverlappedEx::OP_SEND:
            SendCompletion(reinterpret_cast<Session*>(key), ex);
            break;

        default:
            std::cout << "[WORKER][경고] 알 수 없는 작업\n";
            break;
        }
    }
}

// AcceptEx 완료 처리
void IOCP_GameServer::AcceptCompletion(OverlappedEx* ov)
{
    setsockopt(ov->acceptSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenSock, sizeof(listenSock));

    SOCKADDR* localAddr = nullptr;
    SOCKADDR* remoteAddr = nullptr;
    int localLen = 0;
    int remoteLen = 0;

    fnGetAcceptExSockAddr(ov->bufferPtr, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
        &localAddr, &localLen, &remoteAddr, &remoteLen);

    sockaddr_in* clientAddr = (sockaddr_in*)remoteAddr;
    char ipStr[INET_ADDRSTRLEN]{};
    InetNtopA(AF_INET, &clientAddr->sin_addr, ipStr, sizeof(ipStr));

    printf("[ACCEPT] 클라이언트 연결됨: %s:%d\n", ipStr, ntohs(clientAddr->sin_port));

    Session* session = sessionPool.Acquire();
    session->sock = ov->acceptSock;
    session->sessionId = sessionIdGen++;

    {
        std::lock_guard<std::mutex> lock(sessionMtx);
        sessions[session->sessionId] = session;
    }

    CreateIoCompletionPort((HANDLE)session->sock, iocpHandle, (ULONG_PTR)session, 0);
    recvProcessor->PostRecv(session);

    delete[] ov->bufferPtr;
    ovPool.Release(ov);

    PostAccept();
}

// 수신 완료 처리
void IOCP_GameServer::RecvCompletion(Session* session, OverlappedEx* ov, DWORD bytes)
{
    recvProcessor->RecvCompletion(session, ov, bytes);
}

// 송신 완료 처리
void IOCP_GameServer::SendCompletion(Session* session, OverlappedEx* ov)
{
    sendProcessor->SendCompletion(session, ov);
}
