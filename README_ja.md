# camera_manager

`camera_manager` は、ESP-IDF上でカメラを共有フレームソースとして管理するためのコンポーネントです。Kconfigで選択したボードごとのGPIO設定を内部に隠蔽し、フレームコールバック、YUV問い合わせ、YUV基準値による色判定を提供します。

## 役割

| ファイル | 役割 |
|---|---|
| `include/camera_manager.h` | カメラの初期化、開始・停止、フレームコールバック登録 |
| `include/camera_yuv_query.h` | カメラフレームからの画素・周辺平均・一括YUV取得 |
| `include/camera_color.h` | カメラごとのYUV基準値読込と最近傍基準値による色判定 |
| `camera_manager_cfg.h` | 内部用のボードGPIO設定とKconfigによるフレームサイズ定義 |
| `src/*.c` | 公開APIの実装 |

## 対応ボード・フレームサイズ

現在は以下に対応しています。

- XIAO ESP32-S3 Sense
- Freenove ESP32-S3 WROOM1
- QQVGA: 160x120
- QVGA: 320x240
- VGA: 640x480

`camera_get_default_config()` が作る標準設定では、YUV422、Kconfigで選択したフレームサイズ、PSRAM上のフレームバッファ、`CONFIG_CAMERA_FB_COUNT` 個のフレームバッファを使用します。

## Kconfig設定

`idf.py menuconfig` の **Component config -> Camera Configuration** から設定できます。

| 項目 | デフォルト | 目的 |
|---|---:|---|
| Camera board | XIAO ESP32-S3 Sense | ボードごとのカメラGPIOを選択 |
| `CAMERA_FB_COUNT` | 2 | カメラのフレームバッファ数 |
| `CAMERA_XCLK_FREQ` | 20000000 | カメラXCLK周波数(Hz) |
| Camera frame size | QVGA | QQVGA / QVGA / VGAを選択 |

## 初期化の流れ

基本的には次のように使用します。

```c
#include "camera_manager.h"
#include "camera_yuv_query.h"

void app_main(void)
{
    camera_config_t config;
    camera_get_default_config(&config);

    ESP_ERROR_CHECK(camera_init(&config));
    ESP_ERROR_CHECK(camera_yuv_query_init());
    ESP_ERROR_CHECK(camera_start());
}
```

`camera_start()` はバックグラウンドタスクを作成し、フレームを取得するたびに登録済みコールバックを呼び出します。コールバック終了後、フレームバッファはカメラドライバへ返却されます。

## フレームコールバック

毎フレームの処理が必要なコンポーネントは `camera_register_frame_cb()` を使用します。現在の実装では最大4個のコールバックを登録できます。

```c
static void on_frame(camera_fb_t *fb, void *ctx)
{
    // コールバック中にfb->bufを処理する
}

camera_register_frame_cb(on_frame, NULL);
```

コールバック終了後は `camera_fb_t` を保持してはいけません。終了直後にカメラマネージャーがフレームバッファを返却するためです。

## YUV問い合わせ

`camera_yuv_query` は、カメラのパック形式YUV422から扱いやすい問い合わせAPIを提供します。

- `camera_yuv_get_pixel()` : 1画素取得
- `camera_yuv_get_average()` : 指定点周辺の平均YUV取得
- `camera_yuv_get_batch()` : 最大 `CAMERA_YUV_QUERY_MAX_POINTS` 点の一括取得

一括問い合わせは次に利用可能なフレームを待ちます。リクエスト自体が成功しても、個々の座標の成否は `camera_yuv_result_t::status` で確認します。

## 色判定

`camera_color` は最大 `CAMERA_COLOR_MAX_CAMERAS` 個のカメラIDについてYUV基準値を保持します。`camera_color_init()` は `nvs_manager` から基準値を読み込み、保存データが無い場合は組み込みのデフォルト値を使用します。

`camera_color_classify()` はYUV空間の二乗ユークリッド距離を計算し、設定した閾値以内で最も近い色を返します。距離閾値は `camera_color_set_threshold()` で変更できます。

現在のソースでは `UNKNOWN` に加えて、黒、白、赤、緑、青、黄、オレンジの基準値が定義されています。

## 依存コンポーネント

- `esp32-camera`
- `nvs_manager`

このほかESP-IDFのFreeRTOS同期機構や `esp_err` APIを利用します。

## 公開ヘッダー

- `camera_manager.h`
- `camera_yuv_query.h`
- `camera_color.h`

`camera_manager_cfg.h` は非公開ヘッダーで、`PRIV_INCLUDE_DIRS` 経由でコンポーネント内部から利用します。

## 注意事項

このコンポーネントは1つのカメラストリームを複数の利用者で共有する構成を想定しています。同じカメラに対して各利用者が個別に `esp_camera_fb_get()` を呼ぶのではなく、フレームコールバックまたはYUV問い合わせAPIを利用してください。
