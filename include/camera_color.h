#pragma once
/**
 * @file camera_color.h
 * @brief カメラから取得したYUV値を基準色と比較して色分類する公開APIを定義します。基準値はカメラIDごとに管理されます。
 */

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief カメラ色分類で使用する色IDです。
 *
 * 配列インデックスおよびNVSのYUV基準値の並び順として利用されます。
 */
typedef enum {
    CAMERA_COLOR_UNKNOWN = 0,
    CAMERA_COLOR_BLACK,
    CAMERA_COLOR_WHITE,
    CAMERA_COLOR_RED,
    CAMERA_COLOR_GREEN,
    CAMERA_COLOR_BLUE,
    CAMERA_COLOR_YELLOW,
    CAMERA_COLOR_ORANGE,
    CAMERA_COLOR_MAX,   /* 基準値配列のサイズ計算・for文の終端に使う */
} camera_color_id_t;

/** @brief 管理可能なカメラIDの最大スロット数です。IDは0から15を使用します。 */
#define CAMERA_COLOR_MAX_CAMERAS 16

/**
 * @brief 指定カメラのYUV基準値をNVSから読み込みます。
 *
 * データが存在しない場合はコンポーネント内のデフォルト基準値を使用します。
 *
 * @param camera_id 基準値を読み込むカメラIDです。
 */
void camera_load_reference(uint8_t camera_id);

/**
 * @brief 全カメラID分のYUV基準値キャッシュを初期化します。
 *
 * @return 成功時はESP_OKを返します。
 */
esp_err_t camera_color_init(void);

/**
 * @brief 指定カメラの基準YUV値との距離から色を分類します。
 *
 * 許容距離を超えた場合、または無効なカメラIDの場合はUNKNOWNを返します。
 *
 * @param camera_id 判定対象カメラのIDです。
 * @param y Y成分です。
 * @param u U成分です。
 * @param v V成分です。
 * @return 最も近い色のID、または判定不能時のCAMERA_COLOR_UNKNOWNです。
 */
camera_color_id_t camera_color_classify(uint8_t camera_id, uint8_t y, uint8_t u, uint8_t v);

/**
 * @brief 色分類で使用するYUV二乗距離の許容上限を設定します。
 *
 * @param threshold_sq 基準値との二乗距離の最大許容値です。
 */
void camera_color_set_threshold(uint32_t threshold_sq);

#ifdef __cplusplus
}
#endif