#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#define CAMERA_COLOR_MAX_CAMERAS 16

void camera_load_reference(uint8_t camera_id);

esp_err_t camera_color_init(void);

camera_color_id_t camera_color_classify(uint8_t camera_id, uint8_t y, uint8_t u, uint8_t v);

void camera_color_set_threshold(uint32_t threshold_sq);

#ifdef __cplusplus
}
#endif