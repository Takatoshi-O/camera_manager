#include <string.h>
#include <stdlib.h>

#include "camera_yuv_query.h"
#include "camera_manager.h"

#include "esp_log.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "camera_yuv_query";

/*
 * 1回のリクエスト(複数座標分)を表す構造体。
 * camera_register_frame_cb() の user_ctx としてそのまま渡す。
 */
typedef struct {
    SemaphoreHandle_t call_mutex;   /* 複数タスクからの同時呼び出しを直列化 */
    SemaphoreHandle_t done_sem;     /* フレーム処理完了の通知 */

    /* リクエスト内容(呼び出し側がセット) */
    volatile bool pending;
    camera_yuv_point_t points[CAMERA_YUV_QUERY_MAX_POINTS];
    size_t point_count;

    /* 結果(コールバックがセット) */
    volatile esp_err_t overall_result;  /* リクエスト自体の成否 */
    camera_yuv_result_t results[CAMERA_YUV_QUERY_MAX_POINTS];
} yuv_query_ctx_t;

static yuv_query_ctx_t s_ctx;
static bool s_initialized = false;

/* ------------------------------------------------------------------ */
/* YUV422(YUYV packed) から1画素のYUVを取り出す純粋関数                 */
/* lcd_lvgl_cam.c の convert_yuv422_letterboxed と同じレイアウト前提    */
/* ------------------------------------------------------------------ */
static void extract_yuv_pixel(const uint8_t *buf, int width,
                               int x, int y,
                               uint8_t *out_y, uint8_t *out_u, uint8_t *out_v)
{
    int pair_x = x & ~1;   /* YUYVは2px単位で並ぶ */
    const uint8_t *row = buf + (size_t)y * width * 2;
    const uint8_t *p = row + (size_t)pair_x * 2;

    if ((x & 1) == 0) {
        *out_y = p[0];
    } else {
        *out_y = p[2];
    }
    *out_u = p[1];
    *out_v = p[3];
}

/* ------------------------------------------------------------------ */
/* 指定座標を中心とした範囲のYUV平均値を計算する純粋関数                 */
/* ------------------------------------------------------------------ */
static void extract_yuv_average(const uint8_t *buf, int width, int height,
                                 int cx, int cy, int radius,
                                 uint8_t *out_y, uint8_t *out_u, uint8_t *out_v)
{
    int x0 = cx - radius; if (x0 < 0) x0 = 0;
    int y0 = cy - radius; if (y0 < 0) y0 = 0;
    int x1 = cx + radius; if (x1 > width - 1)  x1 = width - 1;
    int y1 = cy + radius; if (y1 > height - 1) y1 = height - 1;

    uint32_t sum_y = 0, sum_u = 0, sum_v = 0;
    uint32_t count = 0;

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            uint8_t py, pu, pv;
            extract_yuv_pixel(buf, width, x, y, &py, &pu, &pv);
            sum_y += py;
            sum_u += pu;
            sum_v += pv;
            count++;
        }
    }

    /* count は (x1>=x0 && y1>=y0) が保証されていれば必ず1以上 */
    *out_y = (uint8_t)(sum_y / count);
    *out_u = (uint8_t)(sum_u / count);
    *out_v = (uint8_t)(sum_v / count);
}

/* ------------------------------------------------------------------ */
/* camera からのフレーム到着コールバック                                */
/* pending なリクエストがあれば処理して done_sem を通知する              */
/* ------------------------------------------------------------------ */

static void on_yuv_frame(camera_fb_t *fb, void *user_ctx)
{
    yuv_query_ctx_t *ctx = (yuv_query_ctx_t *)user_ctx;

    if (!ctx->pending) return;

    int frame_w, frame_h;
    camera_get_frame_size(&frame_w, &frame_h);

    bool frame_ok = (fb->format == PIXFORMAT_YUV422 &&
                      fb->width == frame_w && fb->height == frame_h);

    for (size_t i = 0; i < ctx->point_count; i++) 
    {
        camera_yuv_point_t *pt = &ctx->points[i];
        camera_yuv_result_t *res = &ctx->results[i];

        if (!frame_ok) 
        {
            res->status = ESP_ERR_INVALID_STATE;
            continue;
        }
        if (pt->x < 0 || pt->x >= frame_w || pt->y < 0 || pt->y >= frame_h) 
        {
            res->status = ESP_ERR_INVALID_ARG;
            continue;
        }

        if (pt->radius <= 0) 
        {
            extract_yuv_pixel(fb->buf, frame_w, pt->x, pt->y,
                               &res->y, &res->u, &res->v);
        } 
        else 
        {
            extract_yuv_average(fb->buf, frame_w, frame_h,
                                 pt->x, pt->y, pt->radius,
                                 &res->y, &res->u, &res->v);
        }
        res->status = ESP_OK;
    }

    ctx->overall_result = ESP_OK;
    ctx->pending = false;
    xSemaphoreGive(ctx->done_sem);
}

esp_err_t camera_yuv_query_init(void)
{
    if (s_initialized) return ESP_OK;

    memset(&s_ctx, 0, sizeof(s_ctx));

    s_ctx.call_mutex = xSemaphoreCreateMutex();
    s_ctx.done_sem = xSemaphoreCreateBinary();

    ESP_RETURN_ON_FALSE(s_ctx.call_mutex != NULL && s_ctx.done_sem != NULL,
                        ESP_ERR_NO_MEM, TAG, "semaphore create failed");

    ESP_RETURN_ON_ERROR(
        camera_register_frame_cb(on_yuv_frame, &s_ctx),
        TAG, "camera_register_frame_cb failed"
    );

    s_initialized = true;
    return ESP_OK;
}

/* リクエスト送信〜結果待ちの共通処理 */
esp_err_t camera_yuv_get_batch(const camera_yuv_point_t *points,
                                camera_yuv_result_t *results,
                                size_t count,
                                TickType_t timeout_ticks)
{
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "not initialized");
    ESP_RETURN_ON_FALSE(points != NULL && results != NULL, ESP_ERR_INVALID_ARG, TAG, "NULL pointer");
    ESP_RETURN_ON_FALSE(count >= 1 && count <= CAMERA_YUV_QUERY_MAX_POINTS,
                         ESP_ERR_INVALID_ARG, TAG, "count out of range (1-%d)", CAMERA_YUV_QUERY_MAX_POINTS);

    if (xSemaphoreTake(s_ctx.call_mutex, timeout_ticks) != pdTRUE) 
    {
        return ESP_ERR_TIMEOUT;
    }

    memcpy(s_ctx.points, points, sizeof(camera_yuv_point_t) * count);
    s_ctx.point_count = count;
    s_ctx.pending = true;   /* 次に来るフレームで on_yuv_frame がまとめて処理する */

    esp_err_t ret;
    if (xSemaphoreTake(s_ctx.done_sem, timeout_ticks) == pdTRUE) 
    {
        ret = s_ctx.overall_result;
        memcpy(results, s_ctx.results, sizeof(camera_yuv_result_t) * count);
    } 
    else 
    {
        s_ctx.pending = false; /* タイムアウトしたので取り下げる */
        ret = ESP_ERR_TIMEOUT;
    }

    xSemaphoreGive(s_ctx.call_mutex);
    return ret;
}

/* ------------------------------------------------------------------ */
/* 単一座標用API:内部的にはバッチAPI(count=1)を呼ぶだけ                  */
/* ------------------------------------------------------------------ */

esp_err_t camera_yuv_get_pixel(int x, int y, camera_yuv_result_t *result,
                                TickType_t timeout_ticks)
{
    ESP_RETURN_ON_FALSE(result != NULL, ESP_ERR_INVALID_ARG, TAG, "output is NULL");

    camera_yuv_point_t point = { .x = x, .y = y, .radius = 0 };

    esp_err_t ret = camera_yuv_get_batch(&point, result, 1, timeout_ticks);
    if (ret != ESP_OK) return ret;

    return result->status;
}

esp_err_t camera_yuv_get_average(int x, int y, int radius, camera_yuv_result_t *result,
                                  TickType_t timeout_ticks)
{
    ESP_RETURN_ON_FALSE(radius >= 0, ESP_ERR_INVALID_ARG, TAG, "radius must be >= 0");
    ESP_RETURN_ON_FALSE(result != NULL, ESP_ERR_INVALID_ARG, TAG, "output is NULL");

    camera_yuv_point_t point = { .x = x, .y = y, .radius = radius };

    esp_err_t ret = camera_yuv_get_batch(&point, result, 1, timeout_ticks);
    if (ret != ESP_OK) return ret;

    return result->status;
}