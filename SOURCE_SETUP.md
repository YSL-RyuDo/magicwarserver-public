# Source and build

This repository contains the server implementation and its development history. Account records, password data, personal profile records and generated executables are excluded from every published revision. Commit authorship is retained.

Open the checked-in Visual Studio solution on Windows with the C++ desktop development workload. Game configuration tables remain in the source tree. Create test accounts through the client against your own local server.

## 한국어

서버 구현 코드와 개발 이력을 공개합니다. 계정, 비밀번호, 개인 프로필 데이터와 생성된 실행 파일은 공개 이력에서 제외했습니다. 기존 커밋의 작성자 정보는 유지합니다.

Windows의 Visual Studio C++ 데스크톱 개발 환경에서 저장소의 솔루션을 엽니다. 게임 설정 테이블은 포함되어 있으며 테스트 계정은 로컬 서버에서 클라이언트로 생성합니다.

## 日本語

サーバー実装と開発履歴を公開しています。アカウント、パスワード、個人プロフィール、生成済み実行ファイルは公開履歴から除外し、コミットの著者情報は保持しています。

WindowsのVisual Studio C++デスクトップ開発環境でソリューションを開きます。ゲーム設定テーブルは含まれています。テストアカウントはローカルサーバーに対してクライアントから作成します。

## Arcana Duel database configuration

Provide `ARCANA_DB_HOST`, `ARCANA_DB_USER`, `ARCANA_DB_PASSWORD` and `ARCANA_DB_NAME` as environment variables. The existing connection uses port 3307. Install the MySQL/MariaDB client development library expected by the Visual Studio project. Database records and database credentials are not included.

한국어: DB 접속 정보는 위 환경 변수로 지정합니다. 기존 연결 포트는 3307이며, 프로젝트가 참조하는 MySQL/MariaDB 클라이언트 개발 라이브러리가 필요합니다.

日本語: DB接続情報は上記の環境変数で指定します。既存の接続ポートは3307です。プロジェクトが参照するMySQL/MariaDBクライアント開発ライブラリが必要です。

## Asynchronous DB requests and regression checks

Runtime signup, login, profile, deck reads/replacement and match-result persistence
run on the single DB worker queue. Requests copy their values before leaving packet
dispatch. Completions check the session generation and closing state under the
session lock. Static card/rank tables load before network workers start.

The queue accepts up to 4096 waiting tasks, rejects new work after stopping, and
drains accepted tasks before sessions are released. Work for a closed/recycled
session is skipped; an already-running database write may still commit, but its
response is discarded after disconnect. The three-second match-result delay uses
an owned timer thread; shutdown cancels timers that have not fired and joins the
timer before draining the DB queue.

Account/profile/deck SQL values use prepared statement parameters. Keep UNIQUE
constraints on account IDs and nicknames. The wire header and response layouts are
unchanged; the maximum complete frame is 4096 bytes including its 8-byte header.

Set the MSBuild property `MariaDBRoot` to your installed development package;
the previous Bitnami directory remains the default. Optional `ARCANA_DB_PORT`
(default 3307), `ARCANA_PORT` (default 9000), and `ARCANA_BIND_HOST` (default all
interfaces) support isolated local testing. See [regression tests](tests/README.md)
for a synthetic schema and repeatable commands. The fixture is not a production
migration.

한국어: 로그인·프로필·덱 교체와 회원가입·덱 조회·경기 결과 저장을 DB 워커 큐에서
처리합니다. 연결 종료 후에는 완료 응답을 적용하지 않지만, 이미 시작한 쓰기는
DB에 반영될 수 있습니다. 테스트용 스키마는 실제 배포 DB의 마이그레이션이 아닙니다.
