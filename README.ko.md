# 아르카나 듀얼, Arcana Duel / Server

[English](README.md), [Korean](README.ko.md), [Japanese](README.ja.md)

프로그래머 1명과 그래픽 1명이 함께 만든 멀티플레이 마법 전투 게임입니다.

| 프로젝트 정보 | 내용 |
|---|---|
| 개발 상태 | 개발 기록 |
| 사용 기술 | C++, IOCP, DB worker, Unity |
| 기간과 팀 구성 | 2025.08 ~ 2026.02, 2명, 프로그래머 1명, 그래픽 1명 |
| 내 역할 | 프로그래밍 담당, C++ 서버, Unity 게임 연동 |

[개발 사례 보기](https://ysl-ryudo.github.io/ko/work/magicwar/), [Portfolio](https://ysl-ryudo.github.io/ko/)

[![아르카나 듀얼, Arcana Duel](https://i.ytimg.com/vi/MJo1y09kzd4/hqdefault.jpg)](https://www.youtube.com/watch?v=MJo1y09kzd4)

[Arcana Duel](https://www.youtube.com/watch?v=MJo1y09kzd4), [Early development demo](https://www.youtube.com/watch?v=8sKpA2H9Ib4)

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

| 영역 | 구현 |
|---|---|
| 네트워크 작업 | 연결 수락, IOCP 워커의 송수신 완료 처리 |
| 자원 수명 | 세션, 버퍼, Overlapped I/O 자원 풀 |
| DB 작업 | 별도 DB 워커에서 데이터베이스 요청 처리 |
| 게임 연동 | 마법, 쿨다운, 경기, 프로필, 랭크와 Unity 연동 |

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

## 실행, 코드 확인

Windows C++ 워크로드가 설치된 Visual Studio에서 [projectMagicWar.sln](projectMagicWar.sln)을 여세요. 구성, 데이터 경로, 의존성, 접속 설정을 확인한 뒤 대응하는 클라이언트를 실행하세요. 기존 계정 데이터를 보존하세요.

[magicwar](https://github.com/YSL-RyuDo/magicwar)

## 개발, 검증 범위

과거 구현이며 측정된 동시 접속 수용량은 주장하지 않습니다. 시작 코드에 로컬 DB 설정이 있으므로 실행 전에 격리된 개발 환경에 맞게 확인해야 합니다. 풀 용량 설정을 검증된 플레이어 수로 해석하지 않습니다.

## 관련 저장소

[magicwarserver](https://github.com/YSL-RyuDo/magicwarserver), 비공개, 접근 권한 필요

[magicwar](https://github.com/YSL-RyuDo/magicwar), 비공개, 접근 권한 필요
