# 아르카나 듀얼, Arcana Duel / Server

[English](README.md), [Korean](README.ko.md), [Japanese](README.ja.md)

프로그래머 1명과 그래픽 1명이 함께 만든 멀티플레이 마법 전투 게임입니다.

| 프로젝트 정보 | 내용 |
|---|---|
| 개발 상태 | 개발 기록 |
| 사용 기술 | C++, IOCP, DB worker, Unity |
| 기간과 팀 구성 | 2025.08 ~ 2026.02, 2명, 프로그래머 1명, 그래픽 1명 |
| 내 역할 | 프로그래밍 담당, Unity 게임플레이와 C++ 서버 |

[개발 사례 보기](https://ysl-ryudo.github.io/ko/work/magicwar/), [Portfolio](https://ysl-ryudo.github.io/ko/)

[![아르카나 듀얼, Arcana Duel](https://i.ytimg.com/vi/MJo1y09kzd4/hqdefault.jpg)](https://www.youtube.com/watch?v=MJo1y09kzd4)

[Arcana Duel](https://www.youtube.com/watch?v=MJo1y09kzd4), [Early development demo](https://www.youtube.com/watch?v=8sKpA2H9Ib4)

## 게임 전체에서 맡은 구현

Unity와 C++ 양쪽에서 덱 편성, 캐릭터 조작, 마법 동작, 네트워크 서비스와 경기 결과까지 주요 프로그래밍 흐름을 구현했습니다. 그래픽과 음원은 제작자의 크레딧을 유지하며, 공동으로 수정한 음향과 UI, 다른 팀원이 작성한 크레딧 화면을 제 단독 작업으로 소개하지 않습니다.

**플레이 흐름**

로그인 → 로비와 마법 덱 편성 → 매칭과 로딩 → 이동과 마법 전투 → 승패 → 로비 복귀

소스 링크는 확인한 버전에 고정하고 기여 링크는 제가 작성한 병합 외 변경을 가리킵니다. 공동 파일을 연결했다고 단독 작성이라는 뜻은 아닙니다. 비공개 소스는 저장소 접근 권한이 필요합니다.

### 계정, 로비와 매칭

로그인과 로비의 서비스 및 UI 연결, 로딩과 매칭 취소, 게임 진입을 구현했습니다. 전투 패킷 교환뿐 아니라 경기에 들어가고 나오는 흐름까지 네트워크 응답과 화면 상태를 연결했습니다.

[LoginService.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Login/LoginService.cs), [내 변경 c507717](https://github.com/YSL-RyuDo/magicwar/commit/c5077172f80d9908725b1f1668f4598bb3cdd62b), [LobbyService.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Lobby/LobbyService.cs), [내 변경 c507717](https://github.com/YSL-RyuDo/magicwar/commit/c5077172f80d9908725b1f1668f4598bb3cdd62b), [LoadingSystem.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Loading/LoadingSystem.cs), [내 변경 c507717](https://github.com/YSL-RyuDo/magicwar/commit/c5077172f80d9908725b1f1668f4598bb3cdd62b)

### 마법 덱 4슬롯 편성

카드 선택과 상세 표시, 장착 슬롯 교체를 구현했습니다. SettingSystem은 교체하려는 카드와 현재 덱을 구분하고 변경 성공 뒤 덱을 다시 요청합니다. 로컬 카드 에셋의 표시 정보와 런타임 카드의 게임 수치도 구분했습니다.

[SettingSystem.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/SettingSystem.cs), [내 변경 d14efd6](https://github.com/YSL-RyuDo/magicwar/commit/d14efd652cede62088b9bbebdc6de8534b072340), [CardStatManager.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/CardStatManager.cs), [내 변경 500313d](https://github.com/YSL-RyuDo/magicwar/commit/500313d0294b2d3aca4166f540c7a3bfab1b6c53)

### 마법 시전과 개별 효과

입력과 쿨다운, 투사체 생성과 장판형 마법 분기를 구현했습니다. 화염과 얼음, 전격 투사체, 독과 얼음 영역, 운석 충돌, 회복과 보호막, 은신과 순간이동을 다룹니다. MagicCaster에서 선택 카드와 런타임 수치, 애니메이션과 생성된 마법의 동작을 연결했습니다.

[MagicCaster.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Magic/MagicCaster.cs), [내 변경 a64178a](https://github.com/YSL-RyuDo/magicwar/commit/a64178ace62939442e4a7cb727b7bc2f04663230), [GameManager.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Game/GameManager.cs), [내 변경 2f2abc5](https://github.com/YSL-RyuDo/magicwar/commit/2f2abc5397d4bc8981d862c7a6e8f117cb11138a)

### 이동, 상태 효과와 상대 캐릭터

둔화와 기절, 시전 제약을 반영하는 로컬 이동과 애니메이션, 상대 캐릭터의 이동과 공격, 피격과 사망 표시를 구현했습니다. 일부 효과는 로컬 처리와 표시를 포함하므로 전체 이동을 서버 권위 방식으로 구현했다고 설명하지 않습니다.

[CharacterController2D.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Player/CharacterController2D.cs), [내 변경 a6f8138](https://github.com/YSL-RyuDo/magicwar/commit/a6f81385bb004f7ddae57ac3ea8c29bddd604b9b), [EnemyService.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Enemy/EnemyService.cs), [내 변경 2f2abc5](https://github.com/YSL-RyuDo/magicwar/commit/2f2abc5397d4bc8981d862c7a6e8f117cb11138a)

### 전투 HUD와 경기 종료

체력과 마법 쿨다운 UI, 승패 표시와 로비 복귀를 구현했습니다. 전투와 결과 상태를 화면에 반영하고 서버의 경기, 프로필과 랭크 처리까지 연결했습니다.

[GameUIController.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Game/GameUIController.cs), [내 변경 2f2abc5](https://github.com/YSL-RyuDo/magicwar/commit/2f2abc5397d4bc8981d862c7a6e8f117cb11138a), [MatchManager.cpp (main)](https://github.com/YSL-RyuDo/magicwarserver/blob/54d5e47c5121c31c945aaec206d7e5d4644c9789/projectMagicWar/MatchManager.cpp), [내 변경 5007ff1](https://github.com/YSL-RyuDo/magicwarserver/commit/5007ff1c1c2f0ed204843307d3b3a9c7b8091381)

### 통신 처리와 저장

IOCP 서버의 메시지 경계 복원, 세션 자원과 별도 DB 작업자를 구현했습니다. 아래 설명에서는 TCP 메시지를 복원하는 일과 저장을 기다리는 일을 결과를 사용하는 게임 코드와 왜 분리하는지 다룹니다.

[PacketParser.cpp (main)](https://github.com/YSL-RyuDo/magicwarserver/blob/54d5e47c5121c31c945aaec206d7e5d4644c9789/projectMagicWar/PacketParser.cpp), [내 변경 5007ff1](https://github.com/YSL-RyuDo/magicwarserver/commit/5007ff1c1c2f0ed204843307d3b3a9c7b8091381), [DBWorker.cpp (main)](https://github.com/YSL-RyuDo/magicwarserver/blob/54d5e47c5121c31c945aaec206d7e5d4644c9789/projectMagicWar/DBWorker.cpp), [내 변경 5007ff1](https://github.com/YSL-RyuDo/magicwarserver/commit/5007ff1c1c2f0ed204843307d3b3a9c7b8091381)

## 내 기여

연결 수락, IOCP 워커와 수신, 송신 처리를 구현했습니다.

세션, 버퍼, overlapped 풀과 네트워크 처리에서 분리한 DB 워커를 작업했습니다.

마법, 쿨다운, 매치, 플레이어 프로필, 랭크 관련 게임 서비스를 구현했습니다.

## 기여 근거

[IOCP implementation](https://github.com/YSL-RyuDo/magicwarserver/blob/48daa832f20fab13309e17a4a7af5e283b52d355/projectMagicWar/IOCP_GameServer.cpp)

[Development history / 개발 이력 / 開発履歴](https://github.com/YSL-RyuDo/magicwarserver/commits/main/?author=YSL-RyuDo)

커밋은 설명한 변경의 근거이며 기여율 점수가 아닙니다. 외부 에셋과 팀원 작업의 크레딧은 구분합니다.

## 구현과 설계

멀티플레이 마법 전투에서는 송수신 처리, 재사용할 연결 자원과 DB를 사용하는 게임 서비스가 함께 동작합니다. 서버에서 각 책임이 어떻게 나뉘는지 보여주는 사례입니다.

IOCP 워커, 세션, 버퍼, Overlapped 풀과 별도 DB 워커를 구성했습니다. 마법, 쿨다운 서비스, 경기, 프로필, 랭크와 Unity 게임, 네트워크 연동을 작업했습니다.

## 코드의 동작과 구현 방식을 택한 이유

확인한 구현이 어떤 제약을 다루며 어떤 장단점이 생기는지 설명합니다. 설계 효과에 대한 설명은 코드 분석이며, 비교한 모든 대안을 당시 직접 구현했다는 뜻은 아닙니다. 기여 링크는 공동 프로젝트 안의 담당 변경을 가리키고 소스 링크는 기존 접근 권한을 유지합니다.

### DB 대기와 네트워크 완료 처리를 분리한 이유

**구현 흐름**

DBWorker는 뮤텍스로 보호한 큐로 작업을 받고 조건 변수로 전용 스레드를 깨웁니다. 큐에서 작업을 꺼낼 때만 잠근 뒤 잠금을 해제하고 DB 작업을 실행합니다. 경기 결과 저장은 계정 식별자를 큐 작업에 담아 처리합니다.

**이 구조가 필요한 이유**

DB는 소켓의 준비 여부와 관계없이 저장 장치를 기다릴 수 있습니다. 작업을 분리하면 네트워크 완료 처리 스레드가 DB 호출 안에서 그 시간을 기다리는 결합을 줄일 수 있습니다. 실행 전에 큐 잠금을 풀어야 현재 DB 작업 중에도 다음 요청을 넣을 수 있습니다. 식별자를 전달하면 나중에 실행되는 조회가 원래 접속 객체의 생존에 의존하는 정도도 줄어듭니다.

**대안과 현재 구현의 절충**

DB를 즉시 호출하면 큐가 필요 없지만 저장 대기가 네트워크 작업에 붙습니다. 여러 DB 작업자를 두면 병렬 처리 폭을 늘릴 수 있지만 순서와 연결 관리가 복잡해집니다. 현재 전용 작업자 하나는 더 단순한 분리 방식이며, 큐가 계속 늘어나는 관측은 이 선택을 다시 검토할 근거가 됩니다.

**장단점과 검증 범위**

DB 작업자 하나는 순차 실행을 단순하게 만들지만 요청이 쌓이면 병목이 될 수 있습니다. 큐 용량, 취소, 종료 시 남은 작업을 모두 처리하는 정책은 별도 문제이며, 확인한 Stop 구현을 작업 완료 보장으로 해석하지 않습니다. 이 분리만으로 처리량 향상 수치나 여러 DB 변경의 원자성이 입증되는 것도 아닙니다.

[magicwarserver/DBWorker.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/DBWorker.cpp), [magicwarserver/MatchManager.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/MatchManager.cpp)

### 수신 한 번을 패킷 하나로 취급하지 않는 이유

**구현 흐름**

수신한 바이트를 세션 버퍼에 누적하고 PacketParser가 헤더와 선언된 본문 길이가 모일 때까지 기다립니다. 완성된 메시지만 분배하고 남은 바이트는 다음 분석을 위해 보관합니다. IOCP_Recv는 완료된 수신 버퍼와 overlapped 객체를 풀에 돌려주고 다음 수신을 등록합니다.

**이 구조가 필요한 이유**

TCP는 순서 있는 바이트 스트림이므로 송신 한 번과 수신 한 번의 경계가 같다고 볼 수 없습니다. 따라서 통신과 게임 로직 사이에 메시지 경계를 복원하는 계층이 필요합니다. 풀은 수신 경로의 반복 할당을 다루고, 세션별 누적 버퍼는 아직 덜 온 데이터를 올바른 연결에 묶어 둡니다.

**대안과 현재 구현의 절충**

구분자로 메시지를 나누면 읽기 쉽지만 본문에 구분자가 들어갈 때의 처리 규칙이 필요합니다. 고정 길이는 분석이 단순하지만 공간 낭비나 확장 제약이 생깁니다. 현재 헤더와 본문 길이 방식은 가변 데이터를 담는 대신 길이 검증과 미완성 버퍼 처리가 필요합니다.

**장단점과 검증 범위**

계층을 추가한 만큼 길이, 용량, 객체 수명을 주의해서 관리해야 합니다. 풀의 용량은 할당 설정이며 입증된 동시 접속 인원이 아닙니다. 검증에서는 나뉜 헤더와 본문, 한 수신에 모인 여러 메시지, I/O 대기 중 연결 종료를 다뤄야 합니다.

[magicwarserver/PacketParser.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/PacketParser.cpp), [magicwarserver/IOCP_Recv.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/IOCP_Recv.cpp)

### 마법의 표시 정보와 전투 수치를 나눈 이유

**구현 흐름**

MagicCaster는 시전 전에 로컬 CardData의 프리팹과 서버에서 받은 CardRuntimeData를 확인합니다. IFieldMagic 여부로 장판 배치와 투사체 발사를 나누되 운석은 별도 예외로 처리하고, 생성한 마법에 필요한 런타임 수치를 복사합니다.

**이 구조가 필요한 이유**

프리팹과 그래픽을 바꾸는 이유와 전투 수치를 바꾸는 이유는 다릅니다. 둘을 나누면 로컬 콘텐츠가 외형을 담당하고 런타임 데이터가 공격력과 속도, 반경과 지속 시간을 공급할 수 있습니다. 생성 시점에 값을 넘기므로 해당 마법이 어떤 수치로 시작했는지도 명확해집니다.

**대안과 현재 구현의 절충**

모든 수치를 프리팹에 넣으면 조회는 적지만 밸런스 데이터가 중복될 수 있습니다. 모든 마법을 하나의 switch에서 처리하면 분기는 한눈에 보이지만 개별 동작이 늘수록 커집니다. 현재는 MagicProjectile의 초기화를 공유하고 인터페이스로 장판을 구분하되 일부 예외는 남겨 둔 구조입니다.

**장단점과 검증 범위**

카드 ID와 프리팹, 런타임 레코드가 서로 맞아야 합니다. 운석 ID 예외와 프리팹의 필수 컴포넌트는 검사할 결합 지점입니다. 서버에서 수치를 받는다는 사실만으로 모든 피격과 이동을 서버가 검증한다고 볼 수는 없습니다.

[magicwar/MagicCaster.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Magic/MagicCaster.cs), [magicwar/CardStatManager.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/CardStatManager.cs)

### 덱 교체 요청과 확정된 덱을 구분한 이유

**구현 흐름**

SettingSystem은 슬롯을 고르는 동안 교체할 카드 ID를 임시로 보관합니다. 이전 카드와 새 카드 ID를 전송한 뒤 선택 모드를 끝내고, 교체 성공 응답을 받았을 때 장착 덱을 다시 요청합니다.

**이 구조가 필요한 이유**

요청이 실패하면 화면에서 바꾸려던 내용과 서버에 저장된 덱이 달라집니다. 승인된 상태를 다시 읽으면 클릭을 확정으로 취급하지 않고, 화면 진입 때 사용하는 덱 표시 경로도 재사용할 수 있습니다.

**대안과 현재 구현의 절충**

로컬에서 먼저 바꾸면 즉각 반응하지만 실패 시 되돌리는 처리가 필요합니다. 교체 응답에 새 덱 전체를 담으면 추가 왕복을 줄일 수 있습니다. 현재의 재조회는 기존 경로를 재사용하기 쉽지만 요청과 대기 시간이 추가됩니다.

**장단점과 검증 범위**

확인한 코드는 표시된 스프라이트에서 이전 카드를 찾고 변경 실패를 로그로 남깁니다. 명시적인 슬롯 식별자, 처리 중 표시와 시간 초과 처리를 더하면 규격이 명확해질 수 있습니다. 현재 코드를 완전한 실패 복구로 소개하지 않습니다.

[magicwar/SettingSystem.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/SettingSystem.cs)

## 실행, 코드 확인

Windows C++ 워크로드가 설치된 Visual Studio에서 [projectMagicWar.sln](projectMagicWar.sln)을 여세요. 구성, 데이터 경로, 의존성, 접속 설정을 확인한 뒤 대응하는 클라이언트를 실행하세요. 기존 계정 데이터를 보존하세요.

[magicwar](https://github.com/YSL-RyuDo/magicwar)

## 개발, 검증 범위

과거 구현이며 측정된 동시 접속 수용량은 주장하지 않습니다. 시작 코드에 로컬 DB 설정이 있으므로 실행 전에 격리된 개발 환경에 맞게 확인해야 합니다. 풀 용량 설정을 검증된 플레이어 수로 해석하지 않습니다.

## 관련 저장소

[magicwarserver](https://github.com/YSL-RyuDo/magicwarserver), 비공개, 접근 권한 필요

[magicwar](https://github.com/YSL-RyuDo/magicwar), 비공개, 접근 권한 필요
