# Arcana Duel — Server

[English](README.md) · [Korean](README.ko.md) · [Japanese](README.ja.md)

A multiplayer magic-combat game, built by one programmer and one graphics teammate.

**C++ · IOCP · DB worker · Unity** · Development archive

**Aug 2025 - Feb 2026 · 2 members: 1 programmer, 1 graphics**

Programming: C++ server & Unity game integration

[Explore the case](https://ysl-ryudo.github.io/en/work/magicwar/) · [Portfolio](https://ysl-ryudo.github.io/en/)

[magicwarserver](https://github.com/YSL-RyuDo/magicwarserver) (Private · access required) · [magicwar](https://github.com/YSL-RyuDo/magicwar) (Private · access required)

## System responsibilities

| Area | Implementation |
|---|---|
| Network work | Accept connections; complete receives and sends through IOCP workers |
| Resource lifetime | Reusable session, buffer and overlapped-I/O pools |
| Database work | Database requests handled by a separate DB worker |
| Game integration | Magic, cooldowns, matches, profiles and ranks paired with Unity |

## Development videos

- [Arcana Duel](https://www.youtube.com/watch?v=MJo1y09kzd4)
- [Early development demo](https://www.youtube.com/watch?v=8sKpA2H9Ib4)

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
