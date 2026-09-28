# ファームウェアの独自実装と原版との差分

対象：`baseline-verified-v1`（`36d0c5eaec646596231edf4f8f1b180e797f5732`）。本作品の中心は、既存OS・HAL・学習済みモデルを、時間特性の異なる視覚処理と2コアの提示系へ統合した部分です。OSやYOLOXモデルそのものを新規開発した作品ではありません。

## 1. 追加・統合した機能

開発開始時のアプリケーションは、3タスクでLEDを周期点滅させる構成でした。本作品では次の処理系を構築しています。

| 機能 | 本作品で追加・統合した内容 | 主な実装 |
| --- | --- | --- |
| カメラ入力 | OV5640の制御、MIPI/VIN取得、P108のMIPI経路設定、224×168 RGB565のフレーム公開 | [camera_control.c][camera-control]、[camera_capture.c][capture] |
| USB動画入力 | SELECTによる動画/FPS選択、FAT読出し、Display非依存の再生速度制御、終了後の再挿入と継続するframe ID | [usb_loader.c][usb]、[video_source.c][video] |
| 動き解析 | 縮小グレー画像のブロックマッチング、全体移動の推定、残差から左右活動量を集計 | [motion_task.c][motion] |
| 物体認識 | 既存YOLOX-Tinyを低優先度タスクで実行し、最新の完了結果を共有 | [npu_worker.c][npu] |
| 表示 | 映像、検出枠、活動量、動きベクトルの重畳 | [display_task.c][display] |
| コア間連携 | 動きと物体情報をsnapshotとして共有し、IPCでCPU1へ通知 | [共通定義][common]、両CPUの [ipc_test.c][ipc0] / [ipc_test.c][ipc1] |
| 意味提示 | CPU1で方向・危険度を判定。LEDとコンソールで同じevent/seqを使用し、同期マーカーも出力 | [gvd_policy.c][policy]、[gvd_output.c][output]、[gvd_log.c][log]、[gvd_phy_led.c][phy] |

YOLOの検出結果は表示とsnapshotに使用します。現行の方向・危険度判定は左右活動量に基づき、物体クラスから危険度を推定する方式ではありません。

## 2. μT-Kernelによる実行制御

両CPUとも [app_main.c（CPU0）][main0] / [app_main.c（CPU1）][main1] から各機能のcreate→startを行い、処理はμT-Kernelタスクで実行します。

| コア | タスク優先度（小さい値が高優先度） |
| --- | --- |
| CPU0 | Camera 8、Motion 9、Display 10（初期化中7）、USB 11、NPU 12、IPC 20 |
| CPU1 | IPC 20、方向・LED 25、コンソール 26 |

MotionをNPUより高優先度に置き、詳細認識とは別の周期で動き情報を更新します。CPU1もLED処理とコンソール送信を別タスクにし、UART待ちにLED更新を従属させません。割込みは周辺機器の通知を担当し、通常の画像処理やログ送信はタスク側で行います。

SCI8の出力所有者はCPU1です。BSPのT-Monitorシリアル実装を基にした `Application/demo_console.c` と共通所有設定を使い、CPU0からの重複送信・初期化を避けています。GPT13を共有時刻源とする計測機能も保持しています。

これは優先度・実装構成の説明です。最悪応答時間の保証値や、実歩行での危険回避性能を表すものではありません。

## 3. 提供ソースへの変更

### FSP 6.5.1

両CPUの `ra/fsp/` にあるC・ヘッダ・アセンブリ計207ファイルを同版の元packと比較し、内容変更はCPU0の次の3ファイルでした。

| ファイル | 原版 → 本作品 |
| --- | --- |
| `r_mipi_csi/r_mipi_csi.c` | 割込み設定を直接実行 → 無効ベクタ・無効優先度を除外して有効化。無効ベクタへの無効化処理も回避 |
| `r_vin/r_vin.c` | 同上。VIN側にも同じガードを追加 |
| `r_usb_basic/src/hw/r_usb_dma.c` | DMAバッファの明示的キャッシュ処理なし → 受信前clean/invalidate、受信完了後invalidate、送信前cleanを追加 |

[行単位の差分：fsp-drivers.patch](patches/fsp-drivers.patch)。`r_mipi_phy.c` とFSPの起動・MCU BSPソースは、この比較では原版と一致します。

### μT-Kernel / BSP2

比較元は開発開始時に取り込んだ動作確認済みBSPです。上流の別リリースとの差を本作品の変更として数えず、この導入版からの変更を示します。

| 箇所 | 変更内容 |
| --- | --- |
| 両CPUの `cpu/ra8p1/sysdef.h` | RAMを各 `0xEA000` byteへ分割。CPU0開始 `0x22000000`、CPU1開始 `0x220EA000` |
| CPU1の `include/sys/machine.h` と追加 `ek_ra8p1_cpu1/` 3ヘッダ | Cortex-M33用の選択と250 MHzの時刻設定を追加 |
| CPU1の `config_bsp.h` | CPU0所有のIIC・ADC16H初期化を無効化 |
| CPU0の `devinit.c` | IICインスタンス名を現在のFSP構成へ接続 |
| CPU0の `mtkernel/include/sys/inittask.h` | 空白のみ。スタックサイズ1024 byteは不変 |

[行単位の差分：bsp-port.patch](patches/bsp-port.patch)。比較範囲の `mtkernel/` 本体は上記空白変更以外、改行差を除いて導入版と一致します。

### YOLOX-Tiny / RUHMI Model Zoo

公式モデル集のYOLOX-Tiny `embedded_c/` 24ファイルと比較し、変更は次の4ファイルです。

| ファイル | 変更内容 |
| --- | --- |
| `preprocessing.c/.h` | RGB565入力と、行stride付きフレームの前処理を追加 |
| `src_mcu_npu/sub_0002_invoke.c` | NPU arenaをSDRAMへ配置し、μT-Kernel時刻による実行時間・結果の記録を追加 |
| `src_mcu_npu/kernel_library_int.c` | 配列要素に合わせてポインタ型を `int32_t *` 系へ修正 |

[行単位の差分：yolox-integration.patch](patches/yolox-integration.patch)。残る20ファイルは改行差を除いて一致します。**モデル重み・コマンド列・`model.c`・後処理は独自改変に含めません。** 同梱モデルはこの原版を利用します。

### プロジェクト構成と生成設定

CPU1とSolutionを追加し、`configuration.xml`、`.cproject`、`ra_gen/`、`ra_cfg/`、リンカ・launchを2コア/SDRAM/周辺機器構成へ統合しています。これらは構成入力と生成物であり、上記の提供ドライバ修正とは区別します。実行時は提出コードに同梱した設定を使用します。

## 4. 差分の比較元

| 対象 | 固定した比較元 |
| --- | --- |
| 導入アプリ・BSP | 開発リポジトリ `baseline-ra8p1-bsp2`、`d2652dc1ed9123b2cc5da2a0c1fdb2e163d2e0a2` の `firmware/ra8p1/` |
| FSP | `Renesas.RA.6.5.1.pack`、`Renesas.RA_mcu_ra8p1.6.5.1.pack`、`Arm.Ethos-U-Core-Driver.25.2.0+renesas.0.fsp.6.5.1.pack` の該当ファイル。[公式FSP 6.5.1](https://github.com/renesas/fsp/tree/v6.5.1) |
| YOLOX-Tiny | [Renesas RUHMI Model Zoo](https://github.com/renesas/ruhmi-model-zoo/tree/fe7e84ec6823e9abdfd0553361cf20b1c5dbee42/vision/object_detection/yolox_tiny/embedded_c)、`fe7e84ec6823e9abdfd0553361cf20b1c5dbee42` |

差分はCRLF/LFと行末空白の違いを除いた閲覧用です。対象ファイルのバイト単位のSHA-256を[比較ハッシュ一覧](patches/comparison-sha256.tsv)に保存しています。`same-text` は改行を揃えたときの一致を表します。これらのpatchは改変内容の説明用であり、提出コードに再適用する必要はありません。

[camera-control]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc/Application/camera_control.c
[capture]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc/Application/camera_capture.c
[usb]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc/Application/usb_loader.c
[video]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc/Application/video_source.c
[motion]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc/Application/motion_task.c
[npu]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc/Application/npu_worker.c
[display]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc/Application/display_task.c
[common]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc_Solution/Common/
[ipc0]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc/Application/ipc_test.c
[ipc1]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc_CPU1/Application/ipc_test.c
[policy]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc_CPU1/Application/gvd_policy.c
[output]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc_CPU1/Application/gvd_output.c
[log]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc_CPU1/Application/gvd_log.c
[phy]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc_CPU1/Application/gvd_phy_led.c
[main0]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc/Application/app_main.c
[main1]: ../firmware/evaluations/mtkernel_test7_dualCoreIpc_CPU1/Application/app_main.c
