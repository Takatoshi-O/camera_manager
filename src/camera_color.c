#include <string.h>
#include <limits.h>

#include "camera_color.h"
#include "camera_yuv_query.h"
#include "nvs_manager.h"

#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "camera_color";

/*
 * NVS_MANAGER_CAM_YUV_COLOR_COUNT(デフォルト8) と LUMP_COLOR_MAX(=8) が
 * 一致している前提。もし将来 LUMP_COLOR_* に色を追加して
 * LUMP_COLOR_MAX が変わった場合は、nvs_manager.h を include する前に
 *   #define NVS_MANAGER_CAM_YUV_COLOR_COUNT LUMP_COLOR_MAX
 * を定義してビルドしてください(nvs_manager.h 側の #ifndef ガードで
 * 上書きできる設計になっています)。ここではズレを検知できるよう
 * ビルド時アサートを入れておきます。
 */
_Static_assert(NVS_MANAGER_CAM_YUV_COLOR_COUNT == CAMERA_COLOR_MAX,
               "NVS_MANAGER_CAM_YUV_COLOR_COUNT must match LUMP_COLOR_MAX");

typedef struct {
    uint8_t y, u, v;
} yuv_ref_t;

typedef struct {
    yuv_ref_t colors[CAMERA_COLOR_MAX];
} camera_color_ref_set_t;

/* デフォルト値(全カメラ共通の初期値、実測前提のプレースホルダ) */
static const camera_color_ref_set_t s_default_ref = {
    .colors = {
        [CAMERA_COLOR_UNKNOWN] = { 0,   0,   0   },
        [CAMERA_COLOR_BLACK]   = { 20,  128, 128 },
        [CAMERA_COLOR_WHITE]   = { 235, 128, 128 },
        [CAMERA_COLOR_RED]     = { 76,  84,  255 },
        [CAMERA_COLOR_GREEN]   = { 149, 43,  21  },
        [CAMERA_COLOR_BLUE]    = { 29,  255, 107 },
        [CAMERA_COLOR_YELLOW]  = { 226, 1,   149 },
        [CAMERA_COLOR_ORANGE]  = { 165, 43,  199 },
    }
};

/* カメラごとの基準値(実行時に使うRAMキャッシュ) */
static camera_color_ref_set_t s_reference[CAMERA_COLOR_MAX_CAMERAS];

static uint32_t s_threshold_sq = 4000;

/* ------------------------------------------------------------------ */
/* 1カメラ分をNVSから読み込む。無ければデフォルト値をセットする            */
/* ------------------------------------------------------------------ */

void camera_load_reference(uint8_t camera_id)
{
    nvs_manager_cam_yuv_t nvs_data;
    esp_err_t ret = nvs_manager_read_cam_yuv(camera_id, &nvs_data);

    if (ret == ESP_OK) 
    {
        for (int i = 0; i < CAMERA_COLOR_MAX; i++)
        {
            s_reference[camera_id].colors[i].y = nvs_data.color[i].y;
            s_reference[camera_id].colors[i].u = nvs_data.color[i].u;
            s_reference[camera_id].colors[i].v = nvs_data.color[i].v;
        }
        ESP_LOGI(TAG, "camera %u: loaded calibration from NVS", camera_id);
    } 
    else 
    {
        /* ESP_ERR_NVS_NOT_FOUND を含め、読み込めなければデフォルトを使う */
        s_reference[camera_id] = s_default_ref;
        ESP_LOGI(TAG, "camera %u: using default reference (%s)",
                 camera_id, esp_err_to_name(ret));
    }
}

esp_err_t camera_color_init(void)
{
    /* nvs_manager_init() はアプリ起動時に既に呼ばれている前提
     * (nvs_flash_init を含む初期化はこのモジュールの責務外) */

    for (int i = 0; i < CAMERA_COLOR_MAX_CAMERAS; i++) {
        camera_load_reference((uint8_t)i);
    }

    return ESP_OK;
}

static inline uint32_t squared_distance(const yuv_ref_t *a, uint8_t y, uint8_t u, uint8_t v)
{
    int dy = (int)a->y - (int)y;
    int du = (int)a->u - (int)u;
    int dv = (int)a->v - (int)v;
    return (uint32_t)(dy * dy + du * du + dv * dv);
}

camera_color_id_t camera_color_classify(uint8_t camera_id, uint8_t y, uint8_t u, uint8_t v)
{
    if (camera_id >= CAMERA_COLOR_MAX_CAMERAS) {
        return CAMERA_COLOR_UNKNOWN;
    }

    const camera_color_ref_set_t *ref_set = &s_reference[camera_id];

    camera_color_id_t best_color = CAMERA_COLOR_UNKNOWN;
    uint32_t best_dist = UINT32_MAX;

    for (int i = 1; i < CAMERA_COLOR_MAX; i++) {
        uint32_t dist = squared_distance(&ref_set->colors[i], y, u, v);
        if (dist < best_dist) {
            best_dist = dist;
            best_color = (camera_color_id_t)i;
        }
    }

    if (best_dist > s_threshold_sq) {
        return CAMERA_COLOR_UNKNOWN;
    }

    return best_color;
}

void camera_color_set_threshold(uint32_t threshold_sq)
{
    s_threshold_sq = threshold_sq;
}