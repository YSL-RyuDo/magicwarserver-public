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

## 실행, 코드 확인

Windows C++ 워크로드가 설치된 Visual Studio에서 [projectMagicWar.sln](projectMagicWar.sln)을 여세요. 구성, 데이터 경로, 의존성, 접속 설정을 확인한 뒤 대응하는 클라이언트를 실행하세요. 기존 계정 데이터를 보존하세요.

[magicwar](https://github.com/YSL-RyuDo/magicwar)

## 개발, 검증 범위

과거 구현이며 측정된 동시 접속 수용량은 주장하지 않습니다. 시작 코드에 로컬 DB 설정이 있으므로 실행 전에 격리된 개발 환경에 맞게 확인해야 합니다. 풀 용량 설정을 검증된 플레이어 수로 해석하지 않습니다.

## 관련 저장소

[magicwarserver](https://github.com/YSL-RyuDo/magicwarserver), 비공개, 접근 권한 필요

[magicwar](https://github.com/YSL-RyuDo/magicwar), 비공개, 접근 권한 필요
