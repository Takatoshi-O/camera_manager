#ifndef CAMERA_MANAGER_CFG_H
/**
 * @brief カメラ設定ヘッダーのインクルードガードです。
 */
#define CAMERA_MANAGER_CFG_H
/**
 * @file camera_manager_cfg.h
 * @brief カメラコンポーネント内部で使用するボード別GPIO配置とフレームサイズ設定を定義します。Kconfigの選択結果から実際の設定値を決定します。
 */

/**
 * @file camera_cfg.h
 * @brief ボードごとのカメラピン設定(cameraコンポーネント内部専用)
 */

/* ============================================================
 * ボード選択
 * ============================================================ */

#if defined(CONFIG_CAMERA_BOARD_XIAO_ESP32_S3_SENSE)

/** @brief KconfigでXIAO ESP32-S3 Senseが選択されたことを表す内部値です。 */
    #define CAMERA_BOARD_SELECTED 1

#elif defined(CONFIG_CAMERA_BOARD_FREENOVE_ESP32_S3_WROOM1)

/** @brief KconfigでFreenove ESP32-S3 WROOM1が選択されたことを表す内部値です。 */
    #define CAMERA_BOARD_SELECTED 2

#else

    #error "No supported board selected in menuconfig."

#endif


/* ============================================================
 * XIAO ESP32-S3 Sense
 * ============================================================ */

#if CAMERA_BOARD_SELECTED == 1

/** @brief 選択されたXIAO ESP32-S3 Senseの表示名です。 */
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

/** @brief 選択されたFreenove ESP32-S3 WROOM1の表示名です。 */
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

/** @brief Kconfigで選択されたQQVGAのESPカメラ用フレームサイズです。 */
    #define CAM_FRAME_SIZE FRAMESIZE_QQVGA
    #define CAM_SRC_W      160
    #define CAM_SRC_H      120

#elif defined(CONFIG_CAMERA_FRAME_SIZE_QVGA)

/** @brief Kconfigで選択されたQVGAのESPカメラ用フレームサイズです。 */
    #define CAM_FRAME_SIZE FRAMESIZE_QVGA
    #define CAM_SRC_W      320
    #define CAM_SRC_H      240

#elif defined(CONFIG_CAMERA_FRAME_SIZE_VGA)

/** @brief Kconfigで選択されたVGAのESPカメラ用フレームサイズです。 */
    #define CAM_FRAME_SIZE FRAMESIZE_VGA
    #define CAM_SRC_W      640
    #define CAM_SRC_H      480

#else

    #define CAM_FRAME_SIZE FRAMESIZE_QVGA
    #define CAM_SRC_W      320
    #define CAM_SRC_H      240

#endif

#endif /* CAMERA_MANAGER_CFG_H */