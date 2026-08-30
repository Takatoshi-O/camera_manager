#include <string.h>

#include "camera_manager.h"
#include "camera_manager_cfg.h"   /* 非公開ヘッダ:ピン/フレームサイズ */

#include "esp_log.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "camera";

#define CAMERA_MAX_FRAME_CB 4

typedef struct {
    camera_frame_cb_t cb;
    void *user_ctx;
} cb_entry_t;

static cb_entry_t s_callbacks[CAMERA_MAX_FRAME_CB];
static SemaphoreHandle_t s_cb_mutex = NULL;

static TaskHandle_t s_cam_task_handle = NULL;
static volatile bool s_running = false;
static bool s_initialized = false;

static void cam_task(void *arg)
{
    while (s_running) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        xSemaphoreTake(s_cb_mutex, portMAX_DELAY);
        for (int i = 0; i < CAMERA_MAX_FRAME_CB; i++) {
            if (s_callbacks[i].cb != NULL) {
                s_callbacks[i].cb(fb, s_callbacks[i].user_ctx);
            }
        }
        xSemaphoreGive(s_cb_mutex);

        esp_camera_fb_return(fb);
    }
    s_cam_task_handle = NULL;
    vTaskDelete(NULL);
}

void camera_get_default_config(camera_config_t *out_config)
{
    camera_config_t config = {
        .ledc_channel = LEDC_CHANNEL_0,
        .ledc_timer = LEDC_TIMER_0,

        .pin_d0 = Y2_GPIO_NUM,
        .pin_d1 = Y3_GPIO_NUM,
        .pin_d2 = Y4_GPIO_NUM,
        .pin_d3 = Y5_GPIO_NUM,

        .pin_d4 = Y6_GPIO_NUM,
        .pin_d5 = Y7_GPIO_NUM,
        .pin_d6 = Y8_GPIO_NUM,
        .pin_d7 = Y9_GPIO_NUM,

        .pin_xclk = XCLK_GPIO_NUM,

        .pin_pclk = PCLK_GPIO_NUM,
        .pin_vsync = VSYNC_GPIO_NUM,
        .pin_href = HREF_GPIO_NUM,

        .pin_sccb_sda = SIOD_GPIO_NUM,
        .pin_sccb_scl = SIOC_GPIO_NUM,

        .pin_pwdn     = PWDN_GPIO_NUM,
        .pin_reset    = RESET_GPIO_NUM,
        .xclk_freq_hz = CONFIG_CAMERA_XCLK_FREQ,

        .pixel_format = PIXFORMAT_YUV422,
        .frame_size   = CAM_FRAME_SIZE,
        .fb_count     = CONFIG_CAMERA_FB_COUNT,
        .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
        .fb_location  = CAMERA_FB_IN_PSRAM,
    };

    *out_config = config;
}

void camera_get_frame_size(int *out_width, int *out_height)
{
    *out_width = CAM_SRC_W;
    *out_height = CAM_SRC_H;
}

esp_err_t camera_init(const camera_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");

    if (s_initialized) return ESP_OK;

    s_cb_mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_cb_mutex != NULL, ESP_ERR_NO_MEM, TAG, "mutex create failed");

    memset(s_callbacks, 0, sizeof(s_callbacks));

    ESP_LOGI(TAG, "board=%s", CAMERA_BOARD_NAME);

    ESP_RETURN_ON_ERROR(esp_camera_init(config), TAG, "esp_camera_init failed");

    s_initialized = true;
    return ESP_OK;
}

esp_err_t camera_start(void)
{
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "not initialized");
    if (s_running) return ESP_OK;

    s_running = true;

    BaseType_t result = xTaskCreatePinnedToCore(
        cam_task, "cam_task", 4096, NULL, 5, &s_cam_task_handle, tskNO_AFFINITY
    );

    if (result != pdPASS) {
        s_running = false;
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t camera_stop(void)
{
    s_running = false;
    return ESP_OK;
}

esp_err_t camera_register_frame_cb(camera_frame_cb_t cb, void *user_ctx)
{
    ESP_RETURN_ON_FALSE(cb != NULL, ESP_ERR_INVALID_ARG, TAG, "cb is NULL");
    ESP_RETURN_ON_FALSE(s_cb_mutex != NULL, ESP_ERR_INVALID_STATE, TAG, "not initialized");

    esp_err_t ret = ESP_ERR_NO_MEM;
    xSemaphoreTake(s_cb_mutex, portMAX_DELAY);
    for (int i = 0; i < CAMERA_MAX_FRAME_CB; i++) {
        if (s_callbacks[i].cb == NULL) {
            s_callbacks[i].cb = cb;
            s_callbacks[i].user_ctx = user_ctx;
            ret = ESP_OK;
            break;
        }
    }
    xSemaphoreGive(s_cb_mutex);
    return ret;
}

esp_err_t camera_unregister_frame_cb(camera_frame_cb_t cb, void *user_ctx)
{
    ESP_RETURN_ON_FALSE(s_cb_mutex != NULL, ESP_ERR_INVALID_STATE, TAG, "not initialized");

    esp_err_t ret = ESP_ERR_NOT_FOUND;
    xSemaphoreTake(s_cb_mutex, portMAX_DELAY);
    for (int i = 0; i < CAMERA_MAX_FRAME_CB; i++) {
        if (s_callbacks[i].cb == cb && s_callbacks[i].user_ctx == user_ctx) {
            s_callbacks[i].cb = NULL;
            s_callbacks[i].user_ctx = NULL;
            ret = ESP_OK;
            break;
        }
    }
    xSemaphoreGive(s_cb_mutex);
    return ret;
}