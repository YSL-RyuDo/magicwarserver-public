#pragma once
#include "Session.h"
#include "DBWorkerInstance.h"
#include <functional>

// Work owns request values, never packet-buffer references. Only the completion
// may touch session state, under the same lock used by packet dispatch/reset.
// The server retains sessions until accepted DB work has drained during Stop.
using SessionDBCompletion = std::function<void(Session&)>;
using SessionDBWork = std::function<SessionDBCompletion()>;

inline bool EnqueueSessionDB(Session* session, SessionDBWork work)
{
    if (!session || !g_dbWorker)
        return false;
    std::lock_guard<std::recursive_mutex> lock(session->stateMtx);
    const auto generation = session->generation;
    if (session->closing)
        return false;
    return g_dbWorker->Enqueue([session, generation, work = std::move(work)]() {
        {
            std::lock_guard<std::recursive_mutex> lock(session->stateMtx);
            if (session->closing || session->generation != generation)
                return;
        }
        try
        {
            auto completion = work();
            std::lock_guard<std::recursive_mutex> lock(session->stateMtx);
            if (!session->closing && session->generation == generation && completion)
            {
                completion(*session);
                if (session->closing) shutdown(session->sock, SD_BOTH);
            }
        }
        catch (...)
        {
            std::lock_guard<std::recursive_mutex> lock(session->stateMtx);
            if (session->generation == generation)
            {
                session->closing = true;
                shutdown(session->sock, SD_BOTH);
            }
        }
    });
}
