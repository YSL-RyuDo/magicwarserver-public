# Arcana Duel — IOCP Game Server

[English](README.md) · [Korean](README.ko.md) · [Japanese](README.ja.md)

**C++ · IOCP · DB worker · Unity** · Development archive

[Explore the case](https://ysl-ryudo.github.io/en/work/magicwar/) · [Portfolio](https://ysl-ryudo.github.io/en/)

**Aug 2025 - Feb 2026 · 2 members: 1 programmer, 1 graphics**

Programming: C++ server & Unity game integration


A C++ Windows IOCP server for multiplayer magic combat.

## The engineering problem

Multiplayer magic combat combines receive/send handling, reusable connection resources and database-backed game services. Those responsibilities should be readable as separate parts of the server.

## My implementation

The server uses IOCP workers, session/buffer/overlapped pools and a separate DB worker. My work includes magic/cooldown services, matches, profiles and ranks, with Unity-side game/network integration.

### Implementation overview · conceptual flow

```mermaid
flowchart LR
    N0["Unity client"] --> N1
    N1["IOCP workers / pools"] --> N2
    N2["Game services"] --> N3
    N3["DB worker"]
```

### Development videos

[![Arcana Duel — Arcana Duel](https://i.ytimg.com/vi/MJo1y09kzd4/hqdefault.jpg)](https://www.youtube.com/watch?v=MJo1y09kzd4)

- [Arcana Duel](https://www.youtube.com/watch?v=MJo1y09kzd4)
- [Early development demo](https://www.youtube.com/watch?v=8sKpA2H9Ib4)

Recorded project demonstrations; the contribution section below identifies my work within the game.

## My contribution

- Implemented connection acceptance and IOCP workers with receive/send processing.
- Worked on session/buffer/overlapped pools and a DB worker separated from network processing.
- Implemented game services around magic, cooldowns, matches, player profiles and ranks.

## Contribution evidence

- [IOCP implementation](https://github.com/YSL-RyuDo/magicwarserver/blob/48daa832f20fab13309e17a4a7af5e283b52d355/projectMagicWar/IOCP_GameServer.cpp)
- [Development history / 개발 이력 / 開発履歴](https://github.com/YSL-RyuDo/magicwarserver/commits/main/?author=YSL-RyuDo)

Commits are evidence of the stated changes, not a percentage-of-authorship score. Third-party assets and teammates' work remain separately credited.

## Open and inspect

Open [projectMagicWar.sln](projectMagicWar.sln) in Visual Studio with the Windows C++ workload. Inspect the selected configuration, data paths, dependencies and endpoint settings before starting the matching client. Preserve existing account data.

[magicwar](https://github.com/YSL-RyuDo/magicwar)

## Development and verification scope

Historical implementation; no measured concurrent-user capacity is claimed. The checked-in startup code contains local database configuration: review and supply an appropriate isolated development configuration before running. Pool capacities are implementation settings, not demonstrated player counts.

## How to evaluate the work

Inspect IOCP_GameServer.cpp and the companion client history. Pool sizes are configuration values, not measured concurrency capacity.
