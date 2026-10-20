# Arcana Duel / Server

<!-- portfolio-languages -->
[![English (current)](https://ysl-ryudo.github.io/assets/diagrams/language-en-current.svg)](README.md) [![한국어](https://ysl-ryudo.github.io/assets/diagrams/language-ko.svg)](README.ko.md) [![日本語](https://ysl-ryudo.github.io/assets/diagrams/language-ja.svg)](README.ja.md)
<!-- /portfolio-languages -->

A multiplayer magic-combat game, built by one programmer and one graphics teammate.

| Project | Details |
|---|---|
| Status | Development archive |
| Stack | C++, IOCP, DB worker, Unity |
| Period and team | Aug 2025 ~ Feb 2026, 2 members: 1 programmer, 1 graphics |
| My role | Programming: Unity gameplay and C++ server |

[Explore the case](https://ysl-ryudo.github.io/en/work/magicwar/), [Portfolio](https://ysl-ryudo.github.io/en/)

[![Arcana Duel](https://i.ytimg.com/vi/MJo1y09kzd4/hqdefault.jpg)](https://www.youtube.com/watch?v=MJo1y09kzd4)

[Arcana Duel](https://www.youtube.com/watch?v=MJo1y09kzd4), [Early development demo](https://www.youtube.com/watch?v=8sKpA2H9Ib4)

## Inside the game

[![Four equipped spells and the available card list are connected to the server-stored deck.](https://ysl-ryudo.github.io/assets/gameplay/arcana-deck.webp)](https://ysl-ryudo.github.io/assets/gameplay/arcana-deck.webp)

Four equipped spells and the available card list are connected to the server-stored deck.

[![Two local clients complete matchmaking and exchange spell and damage events in combat.](https://ysl-ryudo.github.io/assets/gameplay/arcana-combat.webp)](https://ysl-ryudo.github.io/assets/gameplay/arcana-combat.webp)

Two local clients complete matchmaking and exchange spell and damage events in combat.

## What I built across the game

I implemented deck editing, character control, spell casting, combat UI and match results in Unity, alongside the C++ network server and database request processing.

[![My implementation flow](https://ysl-ryudo.github.io/assets/diagrams/magicwar-flow-en.svg)](https://ysl-ryudo.github.io/assets/diagrams/magicwar-flow-en.svg)

### Account, lobby and matching

Implemented login and lobby service/UI connections, loading and matching cancellation, and the transition into gameplay. Network responses drive scene-facing state so the game flow includes entering and leaving a match, not only exchanging combat packets.

[LoginService.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Login/LoginService.cs), [My change c507717](https://github.com/YSL-RyuDo/magicwar/commit/c5077172f80d9908725b1f1668f4598bb3cdd62b), [LobbyService.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Lobby/LobbyService.cs), [My change c507717](https://github.com/YSL-RyuDo/magicwar/commit/c5077172f80d9908725b1f1668f4598bb3cdd62b), [LoadingSystem.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Loading/LoadingSystem.cs), [My change c507717](https://github.com/YSL-RyuDo/magicwar/commit/c5077172f80d9908725b1f1668f4598bb3cdd62b)

### Four-slot spell deck

Implemented card selection, detail display and replacement of an equipped slot. SettingSystem separates the pending card from the equipped deck and requests the deck again after a successful change. Card assets provide local presentation while runtime card data supplies gameplay values.

[SettingSystem.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/SettingSystem.cs), [My change d14efd6](https://github.com/YSL-RyuDo/magicwar/commit/d14efd652cede62088b9bbebdc6de8534b072340), [CardStatManager.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/CardStatManager.cs), [My change 500313d](https://github.com/YSL-RyuDo/magicwar/commit/500313d0294b2d3aca4166f540c7a3bfab1b6c53)

### Spell casting and concrete effects

Implemented input and cooldown handling, projectile spawning and field-type spell routing. The game includes fire, ice and electric projectiles, poison and ice areas, meteor impacts, healing, barriers, invisibility and blink. MagicCaster connects the selected card, runtime values, animation and spawned spell behavior.

[MagicCaster.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Magic/MagicCaster.cs), [My change a64178a](https://github.com/YSL-RyuDo/magicwar/commit/a64178ace62939442e4a7cb727b7bc2f04663230), [GameManager.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Game/GameManager.cs), [My change 2f2abc5](https://github.com/YSL-RyuDo/magicwar/commit/2f2abc5397d4bc8981d862c7a6e8f117cb11138a)

### Movement, status effects and remote actors

Implemented local movement and animation with slow, stun and casting restrictions, and remote actor updates for movement, attacks, damage and death. Local input processing and remote presentation use separate paths.

[CharacterController2D.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Player/CharacterController2D.cs), [My change a6f8138](https://github.com/YSL-RyuDo/magicwar/commit/a6f81385bb004f7ddae57ac3ea8c29bddd604b9b), [EnemyService.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Enemy/EnemyService.cs), [My change 2f2abc5](https://github.com/YSL-RyuDo/magicwar/commit/2f2abc5397d4bc8981d862c7a6e8f117cb11138a)

### Combat HUD and match closure

Implemented health and spell-cooldown UI, victory/defeat display and return to the lobby. These screens consume combat and result state; the server also maintains match, profile and rank-related processing.

[GameUIController.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Game/GameUIController.cs), [My change 2f2abc5](https://github.com/YSL-RyuDo/magicwar/commit/2f2abc5397d4bc8981d862c7a6e8f117cb11138a), [MatchManager.cpp (main)](https://github.com/YSL-RyuDo/magicwarserver/blob/54d5e47c5121c31c945aaec206d7e5d4644c9789/projectMagicWar/MatchManager.cpp), [My change 5007ff1](https://github.com/YSL-RyuDo/magicwarserver/commit/5007ff1c1c2f0ed204843307d3b3a9c7b8091381)

### Network transport and persistence

Built the IOCP server's message framing, session resources and separate database worker. The detailed notes explain why TCP message reconstruction and storage waiting are separate concerns from the gameplay code that consumes their results.

[PacketParser.cpp (main)](https://github.com/YSL-RyuDo/magicwarserver/blob/54d5e47c5121c31c945aaec206d7e5d4644c9789/projectMagicWar/PacketParser.cpp), [My change 5007ff1](https://github.com/YSL-RyuDo/magicwarserver/commit/5007ff1c1c2f0ed204843307d3b3a9c7b8091381), [DBWorker.cpp (main)](https://github.com/YSL-RyuDo/magicwarserver/blob/54d5e47c5121c31c945aaec206d7e5d4644c9789/projectMagicWar/DBWorker.cpp), [My change 5007ff1](https://github.com/YSL-RyuDo/magicwarserver/commit/5007ff1c1c2f0ed204843307d3b3a9c7b8091381)

## Contribution evidence

[IOCP implementation](https://github.com/YSL-RyuDo/magicwarserver/blob/48daa832f20fab13309e17a4a7af5e283b52d355/projectMagicWar/IOCP_GameServer.cpp)

[Development history / 개발 이력 / 開発履歴](https://github.com/YSL-RyuDo/magicwarserver/commits/main/?author=YSL-RyuDo)

## Implementation and design

Multiplayer magic combat combines receive/send handling, reusable connection resources and database-backed game services. Those responsibilities should be readable as separate parts of the server.

The server uses IOCP workers, session/buffer/overlapped pools and a separate DB worker. My work includes magic/cooldown services, matches, profiles and ranks, with Unity-side game/network integration.

## How the code works and why it is structured this way

[![Deck replacement](https://ysl-ryudo.github.io/assets/diagrams/magicwar-design-en.svg)](https://ysl-ryudo.github.io/assets/diagrams/magicwar-design-en.svg)

The selected replacement stays pending until the server response. Success triggers a fresh deck read; failure is logged.

### Database latency stays outside network completion work

**Implementation**

DBWorker receives tasks through a mutex-protected queue and wakes a dedicated thread with a condition variable. It removes a task under the lock, then executes the database work after releasing that lock. Match result processing captures account identifiers for the queued task.

**Design rationale**

Database work may wait on storage independently of socket readiness. Separating it prevents a network completion worker from spending that wait inside the database call. Releasing the queue lock before execution also lets producers submit later work while the current task runs. Captured identifiers avoid requiring the original live session object for the queued lookup.

**Alternatives and the current tradeoff**

Executing DB calls inline avoids another queue but couples storage wait to network work. A worker pool can process more DB tasks concurrently, with additional ordering and connection management. One dedicated worker is the simpler separation used here; a growing queue would be evidence to reconsider it.

**Tradeoffs and verification**

One database worker simplifies execution order but can become a bottleneck as requests accumulate. Queue capacity, cancellation and draining pending work during shutdown need explicit policies.

[magicwarserver/DBWorker.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/DBWorker.cpp), [magicwarserver/MatchManager.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/MatchManager.cpp)

### A receive completion is not a game message

**Implementation**

Received bytes accumulate in a session buffer. PacketParser waits for a header and its declared body, dispatches complete messages, then keeps remaining bytes for the next parse. IOCP_Recv returns completed receive buffers and overlapped objects to their pools and posts another receive.

**Design rationale**

TCP carries an ordered byte stream, so one send need not match one receive. Framing belongs between transport and gameplay dispatch. Pooling addresses repeated allocation in the receive path, while per-session accumulation keeps partial data associated with the right connection.

**Alternatives and the current tradeoff**

Delimiter-based messages are easy to inspect but require escaping rules for arbitrary payloads. Fixed-size messages simplify parsing but waste space or limit extensibility. The current header/body length framing supports variable payloads, at the cost of validating lengths and partial buffers.

**Tradeoffs and verification**

Framing requires length and capacity checks and careful object lifetimes. Important cases include split headers/bodies, several messages in one receive and disconnects while I/O is pending.

[magicwarserver/PacketParser.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/PacketParser.cpp), [magicwarserver/IOCP_Recv.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/IOCP_Recv.cpp)

### A spell needs both content and runtime values

**Implementation**

MagicCaster looks up a local CardData prefab and server-fed CardRuntimeData before casting. It uses IFieldMagic to choose field placement or projectile launch, with an explicit meteor exception, and copies the relevant runtime values into the spawned spell.

**Design rationale**

Art/prefab identity and combat parameters change for different reasons. Separating them lets local content define appearance while runtime data supplies damage, speed, radius and duration. Initializing an instance with values also makes its behavior explicit when it is spawned.

**Alternatives and the current tradeoff**

Putting every value in the prefab minimizes lookups but duplicates balance data. One central switch for every spell keeps routing visible but grows with spell-specific behavior. The current design shares initialization through MagicProjectile and marks field behavior with an interface, while retaining some special cases.

**Tradeoffs and verification**

Card IDs, prefabs and runtime records must stay aligned. The meteor ID exception and required prefab components are coupling points to check when adding a spell.

[magicwar/MagicCaster.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Magic/MagicCaster.cs), [magicwar/CardStatManager.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/CardStatManager.cs)

### Replacing a card is a request, not a local fact

**Implementation**

SettingSystem holds a pending card ID while the player selects an equipped slot. It sends the old and new card IDs, clears selection mode, and requests the equipped deck again only when the replacement response reports success.

**Design rationale**

The UI's intended change and the server's stored deck can differ when a request fails. Refreshing from the accepted state avoids treating a click as confirmation and reuses the same deck-to-UI path used on entry.

**Alternatives and the current tradeoff**

An optimistic local swap feels immediate but needs rollback. Returning the full updated deck in the replacement response would avoid a second round trip. The current requery is simpler to reuse but adds a request and a period of waiting.

**Tradeoffs and verification**

The current code identifies the previous card from its displayed sprite and logs replacement failures. Explicit slot IDs, a pending state and timeout handling would make the interaction less dependent on presentation state.

[magicwar/SettingSystem.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/SettingSystem.cs)

## Open and inspect

Open [projectMagicWar.sln](projectMagicWar.sln) in Visual Studio with the Windows C++ workload. Inspect the selected configuration, data paths, dependencies and endpoint settings before starting the matching client. Preserve existing account data.

[magicwar](https://github.com/YSL-RyuDo/magicwar)

## Related repositories

[magicwarserver](https://github.com/YSL-RyuDo/magicwarserver), Private, access required

[magicwar](https://github.com/YSL-RyuDo/magicwar), Private, access required
