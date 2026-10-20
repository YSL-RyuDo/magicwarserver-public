# Arcana Duel / Server

[English](README.md), [Korean](README.ko.md), [Japanese](README.ja.md)

A multiplayer magic-combat game, built by one programmer and one graphics teammate.

| Project | Details |
|---|---|
| Status | Development archive |
| Stack | C++, IOCP, DB worker, Unity |
| Period and team | Aug 2025 ~ Feb 2026, 2 members: 1 programmer, 1 graphics |
| My role | Programming: C++ server & Unity game integration |

[Explore the case](https://ysl-ryudo.github.io/en/work/magicwar/), [Portfolio](https://ysl-ryudo.github.io/en/)

[![Arcana Duel](https://i.ytimg.com/vi/MJo1y09kzd4/hqdefault.jpg)](https://www.youtube.com/watch?v=MJo1y09kzd4)

[Arcana Duel](https://www.youtube.com/watch?v=MJo1y09kzd4), [Early development demo](https://www.youtube.com/watch?v=8sKpA2H9Ib4)

## My contribution

Implemented connection acceptance and IOCP workers with receive/send processing.

Worked on session/buffer/overlapped pools and a DB worker separated from network processing.

Implemented game services around magic, cooldowns, matches, player profiles and ranks.

## Contribution evidence

[IOCP implementation](https://github.com/YSL-RyuDo/magicwarserver/blob/48daa832f20fab13309e17a4a7af5e283b52d355/projectMagicWar/IOCP_GameServer.cpp)

[Development history / 개발 이력 / 開発履歴](https://github.com/YSL-RyuDo/magicwarserver/commits/main/?author=YSL-RyuDo)

Commits are evidence of the stated changes, not a percentage-of-authorship score. Third-party assets and teammates' work remain separately credited.

## Implementation and design

Multiplayer magic combat combines receive/send handling, reusable connection resources and database-backed game services. Those responsibilities should be readable as separate parts of the server.

The server uses IOCP workers, session/buffer/overlapped pools and a separate DB worker. My work includes magic/cooldown services, matches, profiles and ranks, with Unity-side game/network integration.

| Area | Implementation |
|---|---|
| Network work | Accept connections; complete receives and sends through IOCP workers |
| Resource lifetime | Reusable session, buffer and overlapped-I/O pools |
| Database work | Database requests handled by a separate DB worker |
| Game integration | Magic, cooldowns, matches, profiles and ranks paired with Unity |

## How the code works and why it is structured this way

These notes connect the inspected implementation to the constraints it addresses and its tradeoffs. Explanations of design consequences are code analysis, not a claim that every alternative was prototyped. Contribution links identify my changes within team projects; source links retain their original access permissions.

### Database latency stays outside network completion work

**Implementation**

DBWorker receives tasks through a mutex-protected queue and wakes a dedicated thread with a condition variable. It removes a task under the lock, then executes the database work after releasing that lock. Match result processing captures account identifiers for the queued task.

**Design rationale**

Database work may wait on storage independently of socket readiness. Separating it prevents a network completion worker from spending that wait inside the database call. Releasing the queue lock before execution also lets producers submit later work while the current task runs. Captured identifiers avoid requiring the original live session object for the queued lookup.

**Alternatives and the current tradeoff**

Executing DB calls inline avoids another queue but couples storage wait to network work. A worker pool can process more DB tasks concurrently, with additional ordering and connection management. One dedicated worker is the simpler separation used here; a growing queue would be evidence to reconsider it.

**Tradeoffs and verification**

A single DB worker preserves serial execution but can become a queue bottleneck. Queue bounds, cancellation and draining pending tasks during shutdown are separate concerns; the inspected Stop implementation is not a drain guarantee. This separation alone does not establish higher measured throughput or transaction atomicity.

[magicwarserver/DBWorker.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/DBWorker.cpp), [magicwarserver/MatchManager.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/MatchManager.cpp)

### A receive completion is not a game message

**Implementation**

Received bytes accumulate in a session buffer. PacketParser waits for a header and its declared body, dispatches complete messages, then keeps remaining bytes for the next parse. IOCP_Recv returns completed receive buffers and overlapped objects to their pools and posts another receive.

**Design rationale**

TCP carries an ordered byte stream, so one send need not match one receive. Framing belongs between transport and gameplay dispatch. Pooling addresses repeated allocation in the receive path, while per-session accumulation keeps partial data associated with the right connection.

**Alternatives and the current tradeoff**

Delimiter-based messages are easy to inspect but require escaping rules for arbitrary payloads. Fixed-size messages simplify parsing but waste space or limit extensibility. The current header/body length framing supports variable payloads, at the cost of validating lengths and partial buffers.

**Tradeoffs and verification**

The extra layer requires careful length, capacity and lifetime rules. Pool capacity is an allocation setting, not a demonstrated concurrent-player count. Useful validation includes split headers, split bodies, multiple messages in one receive and disconnects with pending I/O.

[magicwarserver/PacketParser.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/PacketParser.cpp), [magicwarserver/IOCP_Recv.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/IOCP_Recv.cpp)

## Open and inspect

Open [projectMagicWar.sln](projectMagicWar.sln) in Visual Studio with the Windows C++ workload. Inspect the selected configuration, data paths, dependencies and endpoint settings before starting the matching client. Preserve existing account data.

[magicwar](https://github.com/YSL-RyuDo/magicwar)

## Development and verification scope

Historical implementation; no measured concurrent-user capacity is claimed. The checked-in startup code contains local database configuration: review and supply an appropriate isolated development configuration before running. Pool capacities are implementation settings, not demonstrated player counts.

## Related repositories

[magicwarserver](https://github.com/YSL-RyuDo/magicwarserver), Private, access required

[magicwar](https://github.com/YSL-RyuDo/magicwar), Private, access required
