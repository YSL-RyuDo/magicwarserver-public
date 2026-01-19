#include "DBWorker.h"

// DB 워커의 실행 상태를 초기화하는 생성자입니다.
DBWorker::DBWorker()
    : running(false)
{
}

// DB 워커 소멸 시 스레드를 안전하게 종료합니다.
DBWorker::~DBWorker()
{
    Stop();
}

// DB 작업 처리를 위한 워커 스레드를 시작합니다.
void DBWorker::Start()
{
    running = true;
    worker = thread(&DBWorker::Run, this);
}

// 워커 스레드를 중단하고 정상 종료를 보장합니다.
void DBWorker::Stop()
{
    if (!running)
        return;

    running = false;
    cv.notify_all();

    if (worker.joinable())
        worker.join();
}

// 실행할 DB 작업을 큐에 등록합니다.
void DBWorker::Enqueue(DBTask task)
{
    {
        lock_guard<mutex> lock(mtx);
        tasks.push(move(task));
    }
    cv.notify_one();
}

// 큐에 쌓인 DB 작업을 순차적으로 처리하는 워커 루프입니다.
void DBWorker::Run()
{
    while (running)
    {
        DBTask task;

        {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [&]() {
                return !tasks.empty() || !running;
                });

            if (!running && tasks.empty())
                return;

            task = move(tasks.front());
            tasks.pop();
        }

        task();
    }
}
