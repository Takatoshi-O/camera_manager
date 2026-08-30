#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 1件の問い合わせ座標(半径0=単一画素、半径>0=周囲平均) */
typedef struct {
    int x;
    int y;
    int radius;
} camera_yuv_point_t;

/* 1件分の結果 */
typedef struct {
    esp_err_t status;   /* この座標単体の成否(範囲外ならESP_ERR_INVALID_ARG等) */
    uint8_t y, u, v;
} camera_yuv_result_t;

/* 一度に問い合わせられる座標数の上限 */
#define CAMERA_YUV_QUERY_MAX_POINTS 16

esp_err_t camera_yuv_query_init(void);

/* 単一座標 */
esp_err_t camera_yuv_get_pixel(int x, int y, camera_yuv_result_t *result,
                                TickType_t timeout_ticks);

esp_err_t camera_yuv_get_average(int x, int y, int radius, camera_yuv_result_t *result,
                                  TickType_t timeout_ticks);

/*
 * 複数座標を1フレームでまとめて取得する。
 * points/results は同じ長さ(count個)の配列。
 * count は 1 〜 CAMERA_YUV_QUERY_MAX_POINTS。
 *
 * 各点の radius=0 なら単一画素、radius>0 ならその点周囲の平均値が
 * results[i] に入る。
 *
 * 戻り値:
 *   ESP_OK              リクエスト自体は正常に処理された
 *                        (個々の座標の成否は results[i].status を見ること)
 *   ESP_ERR_TIMEOUT     timeout_ticks 以内にフレームが来なかった
 *   ESP_ERR_INVALID_ARG count が範囲外、または points/results が NULL
 */
esp_err_t camera_yuv_get_batch(const camera_yuv_point_t *points,
                                camera_yuv_result_t *results,
                                size_t count,
                                TickType_t timeout_ticks);

#ifdef __cplusplus
}
#endif