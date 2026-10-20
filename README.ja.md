# Arcana Duel — Server

[English](README.md) · [Korean](README.ko.md) · [Japanese](README.ja.md)

プログラマー1名とグラフィック担当1名で制作した、マルチプレイヤー魔法バトルゲームです。

**C++ · IOCP · DB worker · Unity** · 開発記録

**2025.08 - 2026.02 · 2名：プログラマー1名、グラフィック1名**

プログラミング担当：C++サーバー・Unityゲーム連携

[開発事例を見る](https://ysl-ryudo.github.io/ja/work/magicwar/) · [Portfolio](https://ysl-ryudo.github.io/ja/)

[magicwarserver](https://github.com/YSL-RyuDo/magicwarserver) (非公開・アクセス権が必要) · [magicwar](https://github.com/YSL-RyuDo/magicwar) (非公開・アクセス権が必要)

## システム別の担当内容

| 領域 | 実装 |
|---|---|
| ネットワーク処理 | 接続受付・IOCPワーカーによる送受信完了処理 |
| リソース寿命 | セッション・バッファー・Overlapped I/Oのプール |
| DB処理 | 専用DBワーカーでデータベース要求を処理 |
| ゲーム連携 | 魔法・クールダウン・試合・プロフィール・ランクとUnityを連携 |

## 開発デモ動画

- [Arcana Duel](https://www.youtube.com/watch?v=MJo1y09kzd4)
- [Early development demo](https://www.youtube.com/watch?v=8sKpA2H9Ib4)

## 担当した内容

- 接続受付、IOCPワーカー、送受信処理を実装しました。
- セッション・バッファー・overlappedのプールと、ネットワーク処理から分離したDBワーカーを担当しました。
- 魔法、クールダウン、試合、プロフィール、ランクに関するゲームサービスを実装しました。

## 貢献の根拠

- [IOCP implementation](https://github.com/YSL-RyuDo/magicwarserver/blob/48daa832f20fab13309e17a4a7af5e283b52d355/projectMagicWar/IOCP_GameServer.cpp)
- [Development history / 개발 이력 / 開発履歴](https://github.com/YSL-RyuDo/magicwarserver/commits/main/?author=YSL-RyuDo)

コミットは記載した変更の根拠であり、貢献率の点数ではありません。外部素材・チームメンバーの貢献は区別します。

## 実行・コードの確認

Windows C++ワークロードを備えたVisual Studioで[projectMagicWar.sln](projectMagicWar.sln)を開いてください。構成・データパス・依存関係・接続設定を確認し、対応するクライアントを起動します。既存アカウントデータは維持してください。

[magicwar](https://github.com/YSL-RyuDo/magicwar)

## 開発・検証範囲

過去の実装で、実測同時接続容量は主張しません。起動コードにローカルDB設定があるため、実行前に隔離された開発環境に合わせて確認してください。プール容量を検証済みプレイヤー数と解釈しないでください。
