#pragma once
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

using namespace std;

class DBWorker
{
public:
    using DBTask = std::function<void()>;

    DBWorker();
    ~DBWorker();

    void Start();
    void Stop();

    void Enqueue(DBTask task);

private:
    void Run();

private:
    thread worker;
    queue<DBTask> tasks;
    mutex mtx;
    condition_variable cv;
    atomic<bool> running;
};
