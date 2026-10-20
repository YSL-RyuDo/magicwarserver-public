# Magicwar — IOCP Game Server

[English](README.md) · [Korean](README.ko.md) · [Japanese](README.ja.md)

**C++ · IOCP · DB worker · Unity** · 개발 기록

[개발 사례 보기](https://ysl-ryudo.github.io/ko/work/magicwar/) · [Portfolio](https://ysl-ryudo.github.io/ko/)


멀티플레이 마법 전투를 위한 C++ Windows IOCP 서버입니다.

## 해결하려던 문제

멀티플레이 마법 전투에서는 송수신 처리, 재사용할 연결 자원과 DB를 사용하는 게임 서비스가 함께 동작합니다. 서버에서 각 책임이 어떻게 나뉘는지 보여주는 사례입니다.

## 내 구현과 설계

IOCP 워커, 세션·버퍼·Overlapped 풀과 별도 DB 워커를 구성했습니다. 마법·쿨다운 서비스, 경기·프로필·랭크와 Unity 게임·네트워크 연동을 작업했습니다.

### 구현 개요 · 개념 흐름도

```mermaid
flowchart LR
    N0["Unity client"] --> N1
    N1["IOCP workers / pools"] --> N2
    N2["Game services"] --> N3
    N3["DB worker"]
```

### 개발 시연 영상

[![Magicwar — Magicwar demo](https://i.ytimg.com/vi/8sKpA2H9Ib4/hqdefault.jpg)](https://www.youtube.com/watch?v=8sKpA2H9Ib4)

- [Magicwar demo](https://www.youtube.com/watch?v=8sKpA2H9Ib4)

촬영 당시의 프로젝트 시연입니다. 게임 안에서 제가 맡은 작업은 아래 기여 항목으로 구분합니다.

## 내 기여

- 연결 수락, IOCP 워커와 수신·송신 처리를 구현했습니다.
- 세션·버퍼·overlapped 풀과 네트워크 처리에서 분리한 DB 워커를 작업했습니다.
- 마법·쿨다운·매치·플레이어 프로필·랭크 관련 게임 서비스를 구현했습니다.

## 기여 근거

- [IOCP implementation](https://github.com/YSL-RyuDo/magicwarserver/blob/48daa832f20fab13309e17a4a7af5e283b52d355/projectMagicWar/IOCP_GameServer.cpp)
- [Development history / 개발 이력 / 開発履歴](https://github.com/YSL-RyuDo/magicwarserver/commits/main/?author=YSL-RyuDo)

커밋은 설명한 변경의 근거이며 기여율 점수가 아닙니다. 외부 에셋과 팀원 작업의 크레딧은 구분합니다.

## 실행·코드 확인

Windows C++ 워크로드가 설치된 Visual Studio에서 [projectMagicWar.sln](projectMagicWar.sln)을 여세요. 구성·데이터 경로·의존성·접속 설정을 확인한 뒤 대응하는 클라이언트를 실행하세요. 기존 계정 데이터를 보존하세요.

[magicwar](https://github.com/YSL-RyuDo/magicwar)

## 개발·검증 범위

과거 구현이며 측정된 동시 접속 수용량은 주장하지 않습니다. 시작 코드에 로컬 DB 설정이 있으므로 실행 전에 격리된 개발 환경에 맞게 확인해야 합니다. 풀 용량 설정을 검증된 플레이어 수로 해석하지 않습니다.

## 구현을 확인하는 방법

IOCP_GameServer.cpp와 대응 클라이언트 이력을 함께 확인할 수 있습니다. 풀 크기는 설정값이며 측정된 동시 접속 수는 아닙니다.
