# 提出物一覧

[公式応募方法](https://www.tron.org/ja/programming_contest-2026/apply/)と[RTOSアプリケーション部門規則](https://www.tron.org/ja/programming_contest-2026/programming_contest_entry-2026/rules-2026/)に対応する提出物は次のとおりです。

| 提出物 | 内容・配置 |
| --- | --- |
| ソースプログラム | `firmware/evaluations/` のCPU0・CPU1・Solutionと同梱依存物 |
| 実行・評価ドキュメント | [実行・評価手順](RUN_AND_EVALUATE.md) |
| 既存ソフトウェアの利用情報 | [第三者ソフトウェア](THIRD_PARTY_SOFTWARE.md)と各ライセンス原文 |
| 紹介スライド（必須） | 作品の目的・構成・独自実装・動作を説明するスライド。配置先 `docs/slides/` |
| 紹介動画（任意） | 動作デモ。配置先 `docs/video/` |

[改変差分](FIRMWARE_CHANGES.md)と[実機確認記録](evidence/baseline-verification.md)は、コードの独自性・再現性を示す補助資料です。

カメラ・LCDはEK-RA8P1開発キット同梱品を標準接続で使用し、独自の追加ハードウェアはありません。カメラ入力で同梱品による動作が可能なため、公式応募方法3-bの機材送付不要条件に該当します。USB動画再生は、配布データをUSBマスストレージへコピーして使用する入力モードです。

提出時は資料を含む配布版のURLを指定し、審査に必要なソースと依存物を取得できる状態にします。既存ソフトウェアの権利処理・提供条件は部門規則1.3に従います。個人の応募手続き書類は公開リポジトリに含めません。
