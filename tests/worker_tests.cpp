#include "../projectMagicWar/DBWorker.h"
#include "../projectMagicWar/SessionDB.h"
#include "../projectMagicWar/DelayedTasks.h"
#include <mysql/mysql.h>
#include <cassert>
#include <future>
#include <iostream>
#include <stdexcept>
#include <vector>

int main()
{
    assert(mysql_library_init(0, nullptr, nullptr) == 0);
    DBWorker worker;
    assert(!worker.Enqueue([] {}));
    assert(worker.Start());
    assert(worker.Start()); // idempotent while running
    std::vector<int> order;
    std::thread::id workerId;
    std::promise<void> entered, release;
    auto gate = release.get_future().share();
    assert(worker.Enqueue([&] { workerId = std::this_thread::get_id(); entered.set_value(); gate.wait(); }));
    entered.get_future().wait();
    for (int i = 0; i < 30; ++i)
        assert(worker.Enqueue([&, i] { order.push_back(i); }));
    assert(worker.Enqueue([] { throw std::runtime_error("synthetic failure"); }));
    assert(worker.Enqueue([&] { order.push_back(30); }));
    release.set_value();
    worker.Stop();
    assert(workerId != std::this_thread::get_id());
    assert(order.size() == 31);
    for (int i = 0; i < 31; ++i) assert(order[i] == i);
    assert(!worker.Enqueue([] {}));

    worker.Start();
    g_dbWorker = &worker;
    Session session;
    std::promise<void> started, finish;
    auto finishGate = finish.get_future().share();
    int completions = 0;
    assert(EnqueueSessionDB(&session, [&]() -> SessionDBCompletion {
        started.set_value();
        finishGate.wait();
        return [&](Session&) { ++completions; };
    }));
    started.get_future().wait();
    session.Reset(); // old completion must not touch a recycled session
    finish.set_value();
    worker.Stop();
    assert(completions == 0);

    worker.Start();
    std::promise<void> started2, finish2;
    auto gate2 = finish2.get_future().share();
    assert(EnqueueSessionDB(&session, [&]() -> SessionDBCompletion {
        started2.set_value(); gate2.wait();
        return [&](Session&) { ++completions; };
    }));
    started2.get_future().wait();
    session.closing = true;
    finish2.set_value();
    worker.Stop();
    assert(completions == 0);
    session.Reset();
    worker.Start();
    assert(EnqueueSessionDB(&session, [&]() -> SessionDBCompletion {
        return [&](Session&) { ++completions; };
    }));
    worker.Stop();
    assert(completions == 1);
    g_dbWorker = nullptr;

    assert(worker.Start());
    std::promise<void> fullEntered, fullRelease;
    auto fullGate = fullRelease.get_future().share();
    assert(worker.Enqueue([&] { fullEntered.set_value(); fullGate.wait(); }));
    fullEntered.get_future().wait();
    int drained = 0;
    for (int i = 0; i < 4096; ++i) assert(worker.Enqueue([&] { ++drained; }));
    assert(!worker.Enqueue([] {}));
    fullRelease.set_value();
    worker.Stop();
    assert(drained == 4096);

    DelayedTasks timers;
    timers.Start();
    std::promise<void> fired;
    timers.Schedule(std::chrono::milliseconds(5), [&] { fired.set_value(); });
    assert(fired.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready);
    bool cancelled = true;
    timers.Schedule(std::chrono::hours(1), [&] { cancelled = false; });
    timers.Stop();
    assert(cancelled);
    mysql_library_end();
    std::cout << "PASS: worker FIFO/drain/reject/exception/restart, session generation/disconnect, timers\n";
}
