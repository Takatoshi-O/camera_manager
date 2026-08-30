#pragma once
/**
 * @file camera_yuv_query.h
 * @brief カメラフレームから指定座標のYUV値を取得する同期型問い合わせAPIを定義します。単一点、周辺平均、一括取得に対応します。
 */

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 1件の問い合わせ座標(半径0=単一画素、半径>0=周囲平均) */
/**
 * @brief 1件のYUV問い合わせ位置を表します。
 *
 * radiusが0なら単一画素、正の値なら周囲領域の平均値を取得します。
 */
typedef struct {
    int x;
    int y;
    int radius;
} camera_yuv_point_t;

/* 1件分の結果 */
/**
 * @brief 1件のYUV問い合わせ結果を表します。
 *
 * statusにはその座標単体の成否が入り、成功時にYUV成分が設定されます。
 */
typedef struct {
    esp_err_t status;   /* この座標単体の成否(範囲外ならESP_ERR_INVALID_ARG等) */
    uint8_t y, u, v;
} camera_yuv_result_t;

/* 一度に問い合わせられる座標数の上限 */
/** @brief 1回のバッチ問い合わせで指定できる座標数の最大値です。 */
#define CAMERA_YUV_QUERY_MAX_POINTS 16

/**
 * @brief YUV問い合わせ機構を初期化し、カメラフレームコールバックへ接続します。
 *
 * @return 初期化結果です。
 */
esp_err_t camera_yuv_query_init(void);

/* 単一座標 */
/**
 * @brief 次に取得されるカメラフレームから指定画素のYUV値を取得します。
 *
 * @param x 横座標です。
 * @param y 縦座標です。
 * @param result 結果の格納先です。
 * @param timeout_ticks フレーム到着を待つ最大Tick数です。
 * @return 問い合わせの成否です。座標範囲外の場合はESP_ERR_INVALID_ARGです。
 */
esp_err_t camera_yuv_get_pixel(int x, int y, camera_yuv_result_t *result,
                                TickType_t timeout_ticks);

/**
 * @brief 指定座標を中心とした周辺領域の平均YUV値を取得します。
 *
 * @param x 中心X座標です。
 * @param y 中心Y座標です。
 * @param radius 平均対象領域の半径です。0以上を指定します。
 * @param result 結果の格納先です。
 * @param timeout_ticks フレーム到着を待つ最大Tick数です。
 * @return 問い合わせの成否です。
 */
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
/**
 * @brief 複数座標を同一フレームからまとめて取得します。
 *
 * @param points 問い合わせ位置の配列です。
 * @param results 各問い合わせ結果の格納先配列です。
 * @param count 問い合わせ件数です。1以上、CAMERA_YUV_QUERY_MAX_POINTS以下を指定します。
 * @param timeout_ticks フレーム到着を待つ最大Tick数です。
 * @return リクエスト全体の処理結果です。個々の座標の成否はresults[i].statusで確認します。
 */
esp_err_t camera_yuv_get_batch(const camera_yuv_point_t *points,
                                camera_yuv_result_t *results,
                                size_t count,
                                TickType_t timeout_ticks);

#ifdef __cplusplus
}
#endif