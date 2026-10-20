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
