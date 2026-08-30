#ifndef CAMERA_MANAGER_CFG_H
#define CAMERA_MANAGER_CFG_H

/**
 * @file camera_cfg.h
 * @brief ボードごとのカメラピン設定(cameraコンポーネント内部専用)
 */

/* ============================================================
 * ボード選択
 * ============================================================ */

#if defined(CONFIG_CAMERA_BOARD_XIAO_ESP32_S3_SENSE)

    #define CAMERA_BOARD_SELECTED 1

#elif defined(CONFIG_CAMERA_BOARD_FREENOVE_ESP32_S3_WROOM1)

    #define CAMERA_BOARD_SELECTED 2

#else

    #error "No supported board selected in menuconfig."

#endif


/* ============================================================
 * XIAO ESP32-S3 Sense
 * ============================================================ */

#if CAMERA_BOARD_SELECTED == 1

    #define CAMERA_BOARD_NAME "XIAO ESP32-S3 Sense"

    #define PWDN_GPIO_NUM    -1
    #define RESET_GPIO_NUM   -1

    #define XCLK_GPIO_NUM    10
    #define SIOD_GPIO_NUM    40
    #define SIOC_GPIO_NUM    39

    #define Y9_GPIO_NUM      48
    #define Y8_GPIO_NUM      11
    #define Y7_GPIO_NUM      12
    #define Y6_GPIO_NUM      14
    #define Y5_GPIO_NUM      16
    #define Y4_GPIO_NUM      18
    #define Y3_GPIO_NUM      17
    #define Y2_GPIO_NUM      15

    #define VSYNC_GPIO_NUM   38
    #define HREF_GPIO_NUM    47
    #define PCLK_GPIO_NUM    13

/* ============================================================
 * Freenove ESP32-S3 WROOM1
 * ============================================================ */

#elif CAMERA_BOARD_SELECTED == 2

    #define CAMERA_BOARD_NAME "Freenove ESP32-S3 WROOM1"

    #define PWDN_GPIO_NUM    -1
    #define RESET_GPIO_NUM   -1

    #define XCLK_GPIO_NUM    15
    #define SIOD_GPIO_NUM     4
    #define SIOC_GPIO_NUM     5

    #define Y9_GPIO_NUM      16
    #define Y8_GPIO_NUM      17
    #define Y7_GPIO_NUM      18
    #define Y6_GPIO_NUM      12
    #define Y5_GPIO_NUM      10
    #define Y4_GPIO_NUM       8
    #define Y3_GPIO_NUM       9
    #define Y2_GPIO_NUM      11

    #define VSYNC_GPIO_NUM    6
    #define HREF_GPIO_NUM     7
    #define PCLK_GPIO_NUM    13

#else

    #error "Invalid board selection."

#endif

/* ----------------------------------------------------
 * KconfigのCamera Frame Sizeから設定を決定
 * ---------------------------------------------------- */

#if defined(CONFIG_CAMERA_FRAME_SIZE_QQVGA)

    #define CAM_FRAME_SIZE FRAMESIZE_QQVGA
    #define CAM_SRC_W      160
    #define CAM_SRC_H      120

#elif defined(CONFIG_CAMERA_FRAME_SIZE_QVGA)

    #define CAM_FRAME_SIZE FRAMESIZE_QVGA
    #define CAM_SRC_W      320
    #define CAM_SRC_H      240

#elif defined(CONFIG_CAMERA_FRAME_SIZE_VGA)

    #define CAM_FRAME_SIZE FRAMESIZE_VGA
    #define CAM_SRC_W      640
    #define CAM_SRC_H      480

#else

    #define CAM_FRAME_SIZE FRAMESIZE_QVGA
    #define CAM_SRC_W      320
    #define CAM_SRC_H      240

#endif

#endif /* CAMERA_MANAGER_CFG_H */