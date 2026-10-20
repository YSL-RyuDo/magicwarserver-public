#pragma once
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <thread>

// One owned timer thread, cancelled and joined before sessions are released.
class DelayedTasks
{
public:
    ~DelayedTasks() { Stop(); }
    void Start()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (worker.joinable()) return;
        stopping = false;
        worker = std::thread([this] { Run(); });
    }
    void Schedule(std::chrono::milliseconds delay, std::function<void()> task)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (stopping) return;
        tasks.emplace(std::chrono::steady_clock::now() + delay, std::move(task));
        cv.notify_one();
    }
    void Stop()
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stopping = true;
            tasks.clear();
        }
        cv.notify_all();
        if (worker.joinable()) worker.join();
    }
private:
    void Run()
    {
        std::unique_lock<std::mutex> lock(mtx);
        while (!stopping)
        {
            if (tasks.empty()) { cv.wait(lock); continue; }
            const auto due = tasks.begin()->first;
            if (std::chrono::steady_clock::now() < due)
            { cv.wait_until(lock, due); continue; }
            auto task = std::move(tasks.begin()->second);
            tasks.erase(tasks.begin());
            lock.unlock();
            try { task(); } catch (...) {}
            lock.lock();
        }
    }
    std::mutex mtx;
    std::condition_variable cv;
    bool stopping = true;
    std::multimap<std::chrono::steady_clock::time_point, std::function<void()>> tasks;
    std::thread worker;
};
