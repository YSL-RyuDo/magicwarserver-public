#include "DBWorker.h"
#include <iostream>
#include <mysql/mysql.h>
#include <future>

DBWorker::DBWorker() : running(false) {}
DBWorker::~DBWorker() { Stop(); }

bool DBWorker::Start()
{
    std::unique_lock<std::mutex> lock(mtx);
    if (running) return true;
    if (worker.joinable()) return false;
    running = true;
    std::promise<bool> initialized;
    auto ready = initialized.get_future();
    worker = std::thread([this, initialized = std::move(initialized)]() mutable {
        const bool ok = mysql_thread_init() == 0;
        initialized.set_value(ok);
        if (!ok) return;
        Run();
        mysql_thread_end();
    });
    if (!ready.get())
    {
        running = false;
        lock.unlock();
        worker.join();
        return false;
    }
    return true;
}

void DBWorker::Stop()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        running = false;
    }
    cv.notify_all();
    if (worker.joinable()) worker.join();
}

bool DBWorker::Enqueue(DBTask task)
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!running || tasks.size() >= 4096) return false;
        tasks.push(std::move(task));
    }
    cv.notify_one();
    return true;
}

void DBWorker::Run()
{
    for (;;)
    {
        DBTask task;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] { return !tasks.empty() || !running; });
            if (tasks.empty() && !running) return;
            task = std::move(tasks.front());
            tasks.pop();
        }
        try { task(); }
        catch (...) { std::cerr << "[DB] task failed; worker remains available\n"; }
    }
}
