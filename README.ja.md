# Arcana Duel / Server

[English](README.md), [Korean](README.ko.md), [Japanese](README.ja.md)

プログラマー1名とグラフィック担当1名で制作した、マルチプレイヤー魔法バトルゲームです。

| プロジェクト情報 | 内容 |
|---|---|
| 開発状況 | 開発記録 |
| 使用技術 | C++, IOCP, DB worker, Unity |
| 期間とチーム構成 | 2025.08 ~ 2026.02, 2名：プログラマー1名、グラフィック1名 |
| 担当 | プログラミング担当、UnityゲームプレイとC++サーバー |

[開発事例を見る](https://ysl-ryudo.github.io/ja/work/magicwar/), [Portfolio](https://ysl-ryudo.github.io/ja/)

[![Arcana Duel](https://i.ytimg.com/vi/MJo1y09kzd4/hqdefault.jpg)](https://www.youtube.com/watch?v=MJo1y09kzd4)

[Arcana Duel](https://www.youtube.com/watch?v=MJo1y09kzd4), [Early development demo](https://www.youtube.com/watch?v=8sKpA2H9Ib4)

## ゲーム全体で担当した実装

UnityとC++の両側でデッキ編成、キャラクター操作、魔法動作、通信機能、試合結果まで主要なプログラミングを実装しました。画像・音源の制作者を尊重し、共同の音声・UI変更や別メンバーによるクレジット画面を単独成果には含めません。

**プレイの流れ**

ログイン → ロビーと魔法デッキ編成 → マッチングとロード → 移動と魔法戦闘 → 勝敗 → ロビー復帰

ソースリンクは確認した版に固定し、担当リンクは自分のマージ以外の変更を示します。共同ファイルは単独著作を意味しません。非公開ソースには閲覧権限が必要です。

### アカウント、ロビーとマッチング

ログイン・ロビーのサービスとUI接続、ロードとマッチング取消、ゲーム開始を実装しました。戦闘パケットだけでなく、試合への出入りも通信応答と画面状態でつなぎました。

[LoginService.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Login/LoginService.cs), [担当変更 c507717](https://github.com/YSL-RyuDo/magicwar/commit/c5077172f80d9908725b1f1668f4598bb3cdd62b), [LobbyService.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Lobby/LobbyService.cs), [担当変更 c507717](https://github.com/YSL-RyuDo/magicwar/commit/c5077172f80d9908725b1f1668f4598bb3cdd62b), [LoadingSystem.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Loading/LoadingSystem.cs), [担当変更 c507717](https://github.com/YSL-RyuDo/magicwar/commit/c5077172f80d9908725b1f1668f4598bb3cdd62b)

### 4スロットの魔法デッキ

カード選択、詳細表示、装備スロット交換を実装しました。SettingSystemは交換候補と現在のデッキを分け、変更成功後に再取得します。ローカルカードの表示情報と実行時のゲーム数値も区別しました。

[SettingSystem.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/SettingSystem.cs), [担当変更 d14efd6](https://github.com/YSL-RyuDo/magicwar/commit/d14efd652cede62088b9bbebdc6de8534b072340), [CardStatManager.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/CardStatManager.cs), [担当変更 500313d](https://github.com/YSL-RyuDo/magicwar/commit/500313d0294b2d3aca4166f540c7a3bfab1b6c53)

### 魔法発動と個別効果

入力とクールダウン、弾生成とフィールド型魔法への分岐を実装しました。炎・氷・雷の弾、毒・氷の領域、隕石、回復・バリア、透明化・瞬間移動を扱います。MagicCasterがカード、実行時数値、アニメーション、生成魔法を接続します。

[MagicCaster.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Magic/MagicCaster.cs), [担当変更 a64178a](https://github.com/YSL-RyuDo/magicwar/commit/a64178ace62939442e4a7cb727b7bc2f04663230), [GameManager.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Game/GameManager.cs), [担当変更 2f2abc5](https://github.com/YSL-RyuDo/magicwar/commit/2f2abc5397d4bc8981d862c7a6e8f117cb11138a)

### 移動、状態効果と相手キャラクター

スロー、スタン、詠唱制約を反映するローカル移動・アニメーションと、相手の移動・攻撃・被弾・死亡表示を実装しました。一部はローカル処理や表示を含み、全移動をサーバー権威型と表現しません。

[CharacterController2D.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Player/CharacterController2D.cs), [担当変更 a6f8138](https://github.com/YSL-RyuDo/magicwar/commit/a6f81385bb004f7ddae57ac3ea8c29bddd604b9b), [EnemyService.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Enemy/EnemyService.cs), [担当変更 2f2abc5](https://github.com/YSL-RyuDo/magicwar/commit/2f2abc5397d4bc8981d862c7a6e8f117cb11138a)

### 戦闘HUDと試合終了

体力・魔法クールダウンUI、勝敗表示、ロビー復帰を実装しました。戦闘・結果状態を画面に反映し、サーバーの試合・プロフィール・ランク処理も接続しました。

[GameUIController.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Game/GameUIController.cs), [担当変更 2f2abc5](https://github.com/YSL-RyuDo/magicwar/commit/2f2abc5397d4bc8981d862c7a6e8f117cb11138a), [MatchManager.cpp (main)](https://github.com/YSL-RyuDo/magicwarserver/blob/54d5e47c5121c31c945aaec206d7e5d4644c9789/projectMagicWar/MatchManager.cpp), [担当変更 5007ff1](https://github.com/YSL-RyuDo/magicwarserver/commit/5007ff1c1c2f0ed204843307d3b3a9c7b8091381)

### 通信処理と永続化

IOCPサーバーのメッセージ境界復元、セッション資源、専用DBワーカーを実装しました。後述ではTCPメッセージ復元と保存待ちを、結果を利用するゲーム処理から分ける理由を説明します。

[PacketParser.cpp (main)](https://github.com/YSL-RyuDo/magicwarserver/blob/54d5e47c5121c31c945aaec206d7e5d4644c9789/projectMagicWar/PacketParser.cpp), [担当変更 5007ff1](https://github.com/YSL-RyuDo/magicwarserver/commit/5007ff1c1c2f0ed204843307d3b3a9c7b8091381), [DBWorker.cpp (main)](https://github.com/YSL-RyuDo/magicwarserver/blob/54d5e47c5121c31c945aaec206d7e5d4644c9789/projectMagicWar/DBWorker.cpp), [担当変更 5007ff1](https://github.com/YSL-RyuDo/magicwarserver/commit/5007ff1c1c2f0ed204843307d3b3a9c7b8091381)

## 担当した内容

接続受付、IOCPワーカー、送受信処理を実装しました。

セッション・バッファー・overlappedのプールと、ネットワーク処理から分離したDBワーカーを担当しました。

魔法、クールダウン、試合、プロフィール、ランクに関するゲームサービスを実装しました。

## 貢献の根拠

[IOCP implementation](https://github.com/YSL-RyuDo/magicwarserver/blob/48daa832f20fab13309e17a4a7af5e283b52d355/projectMagicWar/IOCP_GameServer.cpp)

[Development history / 개발 이력 / 開発履歴](https://github.com/YSL-RyuDo/magicwarserver/commits/main/?author=YSL-RyuDo)

コミットは記載した変更の根拠であり、貢献率の点数ではありません。外部素材・チームメンバーの貢献は区別します。

## 実装と設計

マルチプレイヤー魔法戦闘では送受信、再利用する接続リソース、DBを使うゲームサービスが共存します。サーバー内の責務の分け方を示す事例です。

IOCPワーカー、セッション・バッファー・Overlappedプール、独立したDBワーカーを構成しました。魔法・クールダウン、試合・プロフィール・ランク、Unityとの連携を担当しました。

## コードの動作と実装構造の理由

確認した実装が扱う制約と利点・代償を説明します。設計効果の説明はコード分析であり、比較する全代案を当時試作したという意味ではありません。担当リンクは共同制作内の変更を示し、ソースの閲覧権限は維持します。

### DB待機とネットワーク完了処理を分ける理由

**実装の流れ**

DBWorkerはミューテックスで保護したキューにタスクを受け取り、条件変数で専用スレッドを起こします。取り出し時だけロックし、解除してからDB処理を実行します。試合結果の保存ではアカウント識別子をタスクに渡します。

**この構造が必要な理由**

DBはソケットの状態と無関係にストレージを待つことがあります。分離によりネットワーク完了スレッドをDB呼出し内で待たせる結合を減らせます。実行前にキューのロックを解除すれば、その処理中も次の要求を投入できます。識別子の受渡しは、遅れて実行される検索の元セッションへの依存も減らします。

**代案と現在の実装の折合い**

DBの直接呼出しはキュー不要ですが保存待ちと通信処理を結びます。複数ワーカーは並列度を上げる代わりに順序と接続の管理を複雑にします。専用ワーカー一つはここで使う単純な分離であり、キューの継続的増加は再検討の根拠になります。

**利点・代償と検証範囲**

単一DBワーカーは順次実行を簡単にしますが、要求がたまるとボトルネックになります。キュー上限、取消、終了時の残タスク完了は別の課題であり、確認したStop実装は全処理完了の保証ではありません。この分離だけでは実測スループットや複数DB更新の原子性も示せません。

[magicwarserver/DBWorker.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/DBWorker.cpp), [magicwarserver/MatchManager.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/MatchManager.cpp)

### 1回の受信を1メッセージと見なさない理由

**実装の流れ**

受信バイトをセッションバッファに蓄積し、PacketParserはヘッダーと指定された本文がそろうまで待ちます。完成したメッセージだけを振り分け、残りは次回に保持します。IOCP_Recvは受信バッファとoverlappedオブジェクトをプールへ戻し、次の受信を登録します。

**この構造が必要な理由**

TCPは順序のあるバイトストリームなので、送信と受信の境界は一致しません。通信とゲーム処理の間でメッセージ境界を復元する層が必要です。プールは受信経路の反復割当てを扱い、セッションごとの蓄積は未完のデータを正しい接続に対応させます。

**代案と現在の実装の折合い**

区切り文字方式は読みやすい反面、本文内の区切りのエスケープが必要です。固定長は解析が単純ですが無駄な領域や拡張制限が生じます。現在のヘッダー・本文長方式は可変データを扱う代わりに長さと未完バッファの検証が必要です。

**利点・代償と検証範囲**

層を増やす分、長さ、容量、オブジェクト寿命の管理が必要です。プール容量は割当て設定であり、実証された同時接続人数ではありません。ヘッダー分割、本文分割、複数メッセージの同時受信、I/O待機中の切断が検証対象になります。

[magicwarserver/PacketParser.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/PacketParser.cpp), [magicwarserver/IOCP_Recv.cpp](https://github.com/YSL-RyuDo/magicwarserver/blob/8089751ae5136876d6fe133fdb980ef24c97684c/projectMagicWar/IOCP_Recv.cpp)

### 魔法の表示情報と戦闘数値を分ける理由

**実装の流れ**

MagicCasterは発動前にローカルCardDataのプレハブとサーバー由来のCardRuntimeDataを確認します。IFieldMagicで領域配置と弾発射を分け、隕石は明示的な例外とし、必要な実行時数値を生成魔法へコピーします。

**この構造が必要な理由**

プレハブ・画像と戦闘数値は変更理由が異なります。分離すればローカル内容が見た目を、実行時データが攻撃力・速度・半径・時間を担います。生成時に値を渡すことで開始条件も明確です。

**代案と現在の実装の折合い**

全数値をプレハブへ置けば参照は減りますが調整データが重複します。全魔法を一つのswitchに集めると経路は見えますが個別動作とともに肥大化します。現在はMagicProjectileの初期化を共有し、インターフェースで領域型を区別しつつ例外も残します。

**利点・代償と検証範囲**

カードID、プレハブ、実行時レコードの互換性が必要です。隕石ID例外と必須コンポーネントは検証点です。サーバー由来の数値だけでは全命中・移動のサーバー検証は証明できません。

[magicwar/MagicCaster.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Magic/MagicCaster.cs), [magicwar/CardStatManager.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/CardStatManager.cs)

### デッキ変更要求と確定状態を区別する

**実装の流れ**

SettingSystemはスロット選択中に候補カードIDを保持します。旧・新IDを送り選択モードを終了し、成功応答時に装備デッキを再取得します。

**この構造が必要な理由**

失敗時はUIの意図と保存済みデッキが異なります。承認状態を再取得すればクリックを確定扱いせず、画面開始時と同じ表示経路を再利用できます。

**代案と現在の実装の折合い**

楽観的な即時交換には巻戻しが必要です。変更応答に新デッキ全体を含めれば追加往復を省けます。現在の再取得は既存経路を再利用しやすい代わりに要求と待ち時間が増えます。

**利点・代償と検証範囲**

確認コードは表示スプライトから旧カードを探し、失敗をログへ出します。明示的スロットID、処理中表示、タイムアウトがあれば規約が明確になります。完全な失敗復旧とは表現しません。

[magicwar/SettingSystem.cs (main)](https://github.com/YSL-RyuDo/magicwar/blob/20cff216e385494429a4629127ef85f8732a82db/Assets/Script/Setting/SettingSystem.cs)

## 実行・コードの確認

Windows C++ワークロードを備えたVisual Studioで[projectMagicWar.sln](projectMagicWar.sln)を開いてください。構成・データパス・依存関係・接続設定を確認し、対応するクライアントを起動します。既存アカウントデータは維持してください。

[magicwar](https://github.com/YSL-RyuDo/magicwar)

## 開発・検証範囲

過去の実装で、実測同時接続容量は主張しません。起動コードにローカルDB設定があるため、実行前に隔離された開発環境に合わせて確認してください。プール容量を検証済みプレイヤー数と解釈しないでください。

## 関連リポジトリ

[magicwarserver](https://github.com/YSL-RyuDo/magicwarserver), 非公開・アクセス権が必要

[magicwar](https://github.com/YSL-RyuDo/magicwar), 非公開・アクセス権が必要
