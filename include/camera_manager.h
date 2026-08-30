#pragma once
/**
 * @file camera_manager.h
 * @brief カメラの初期化・開始・停止と、取得フレームを受け取るコールバック登録を提供します。
 */

#include "esp_err.h"
#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 新しいカメラフレームを受け取ったときに呼び出されるコールバック型です。
 *
 * @param fb カメラフレームバッファへのポインタです。コールバック終了後はコンポーネントが返却します。
 * @param user_ctx 登録時に指定したユーザーコンテキストです。
 */
typedef void (*camera_frame_cb_t)(camera_fb_t *fb, void *user_ctx);

/**
 * @brief ESPカメラドライバとコールバック管理機構を初期化します。
 *
 * @param config esp_cameraに渡すカメラ設定です。
 * @return 初期化結果です。
 */
esp_err_t camera_init(const camera_config_t *config);
/**
 * @brief カメラフレーム取得タスクを開始します。
 *
 * @return 成功時はESP_OK、未初期化時はESP_ERR_INVALID_STATEです。
 */
esp_err_t camera_start(void);
/**
 * @brief カメラフレーム取得タスクの実行を停止します。
 *
 * @return 常にESP_OKを返します。
 */
esp_err_t camera_stop(void);

/**
 * @brief フレーム到着時に呼び出すコールバックを登録します。
 *
 * 同時に登録できるコールバック数にはコンポーネント内部の上限があります。
 *
 * @param cb 登録するコールバック関数です。
 * @param user_ctx コールバックへ渡すユーザーコンテキストです。
 * @return 登録結果です。空きスロットがなければESP_ERR_NO_MEMを返します。
 */
esp_err_t camera_register_frame_cb(camera_frame_cb_t cb, void *user_ctx);
/**
 * @brief 登録済みのフレームコールバックを解除します。
 *
 * @param cb 解除対象のコールバック関数です。
 * @param user_ctx 登録時に使用したユーザーコンテキストです。
 * @return 解除できた場合はESP_OK、該当登録がなければESP_ERR_NOT_FOUNDです。
 */
esp_err_t camera_unregister_frame_cb(camera_frame_cb_t cb, void *user_ctx);

/*
 * Kconfigの設定(ボード・フレームサイズ)から camera_config_t を組み立てる。
 * pixel_format は YUV422 固定。呼び出し側はピン配置等を一切意識しなくてよい。
 */
/**
 * @brief Kconfigとボード設定から標準のcamera_config_tを作成します。
 *
 * YUV422、設定済みフレームサイズ、PSRAM上のフレームバッファなどを設定します。
 *
 * @param out_config 作成した設定を書き込む領域です。
 */
void camera_get_default_config(camera_config_t *out_config);

/*
 * Kconfigで選択されたフレームサイズの実解像度を取得する。
 * camera_init() 前でも呼び出し可能(純粋にビルド時設定を返すだけ)。
 */
/**
 * @brief Kconfigで選択されたカメラの実解像度を取得します。
 *
 * @param out_width 横幅の格納先です。
 * @param out_height 高さの格納先です。
 */
void camera_get_frame_size(int *out_width, int *out_height);

#ifdef __cplusplus
}
#endif