#pragma once

#include "esp_err.h"
#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*camera_frame_cb_t)(camera_fb_t *fb, void *user_ctx);

esp_err_t camera_init(const camera_config_t *config);
esp_err_t camera_start(void);
esp_err_t camera_stop(void);

esp_err_t camera_register_frame_cb(camera_frame_cb_t cb, void *user_ctx);
esp_err_t camera_unregister_frame_cb(camera_frame_cb_t cb, void *user_ctx);

/*
 * Kconfigの設定(ボード・フレームサイズ)から camera_config_t を組み立てる。
 * pixel_format は YUV422 固定。呼び出し側はピン配置等を一切意識しなくてよい。
 */
void camera_get_default_config(camera_config_t *out_config);

/*
 * Kconfigで選択されたフレームサイズの実解像度を取得する。
 * camera_init() 前でも呼び出し可能(純粋にビルド時設定を返すだけ)。
 */
void camera_get_frame_size(int *out_width, int *out_height);

#ifdef __cplusplus
}
#endif