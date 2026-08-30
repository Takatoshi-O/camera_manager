# camera_manager

`camera_manager` is an ESP-IDF component for managing an ESP camera as a shared frame source. It hides board-specific pin assignments behind Kconfig settings and provides callback-based frame distribution, YUV sampling, and camera-color classification.

## Responsibilities

| File | Responsibility |
|---|---|
| `include/camera_manager.h` | Camera initialization, start/stop control, and frame callback registration |
| `include/camera_yuv_query.h` | Pixel, local-average, and batch YUV queries from camera frames |
| `include/camera_color.h` | Per-camera YUV reference loading and nearest-reference color classification |
| `camera_manager_cfg.h` | Internal board pin mapping and Kconfig-selected frame-size definitions |
| `src/*.c` | Implementation of the public APIs |

## Supported boards and frame sizes

The current configuration supports:

- XIAO ESP32-S3 Sense
- Freenove ESP32-S3 WROOM1
- QQVGA: 160x120
- QVGA: 320x240
- VGA: 640x480

The default camera configuration produced by `camera_get_default_config()` uses YUV422 pixel format, the Kconfig-selected frame size, PSRAM frame buffers, and `CONFIG_CAMERA_FB_COUNT` frame buffers.

## Kconfig

Open `idf.py menuconfig` and select **Component config -> Camera Configuration**.

| Option | Default | Purpose |
|---|---:|---|
| Camera board | XIAO ESP32-S3 Sense | Selects the board-specific camera GPIO mapping |
| `CAMERA_FB_COUNT` | 2 | Number of camera frame buffers |
| `CAMERA_XCLK_FREQ` | 20000000 | Camera XCLK frequency in Hz |
| Camera frame size | QVGA | Selects QQVGA, QVGA, or VGA |

## Initialization flow

A typical application flow is:

```c
#include "camera_manager.h"
#include "camera_yuv_query.h"

void app_main(void)
{
    camera_config_t config;
    camera_get_default_config(&config);

    ESP_ERROR_CHECK(camera_init(&config));
    ESP_ERROR_CHECK(camera_yuv_query_init());
    ESP_ERROR_CHECK(camera_start());
}
```

`camera_start()` creates a background task that repeatedly obtains a frame, invokes all registered frame callbacks, and returns the frame buffer after callbacks complete.

## Frame callbacks

Use `camera_register_frame_cb()` when another component needs to process every captured frame. Up to four callback slots are available in the current implementation.

```c
static void on_frame(camera_fb_t *fb, void *ctx)
{
    // Process fb->buf while the callback is running.
}

camera_register_frame_cb(on_frame, NULL);
```

The callback must not retain the `camera_fb_t` pointer after it returns because the camera manager returns the frame buffer to `esp_camera` immediately afterward.

## YUV queries

`camera_yuv_query` converts the camera's YUV422 packed frame data into convenient point queries.

- `camera_yuv_get_pixel()` reads one pixel.
- `camera_yuv_get_average()` reads the average YUV value around a center point.
- `camera_yuv_get_batch()` handles up to `CAMERA_YUV_QUERY_MAX_POINTS` points from one frame.

A batch request waits for the next available frame. The returned `camera_yuv_result_t::status` is checked per point, so some points may fail while the request itself succeeds.

## Color classification

`camera_color` keeps a YUV reference set for up to `CAMERA_COLOR_MAX_CAMERAS` camera IDs. `camera_color_init()` loads each reference set from `nvs_manager`; missing data falls back to the built-in default references.

`camera_color_classify()` calculates the squared Euclidean distance in YUV space and returns the closest color when it is within the configured threshold. `camera_color_set_threshold()` changes that global squared-distance threshold.

The source currently defines references for black, white, red, green, blue, yellow, and orange in addition to `UNKNOWN`.

## Dependencies

Declared dependencies:

- `esp32-camera`
- `nvs_manager`

The component also uses ESP-IDF FreeRTOS synchronization primitives and `esp_err` APIs.

## Public headers

- `camera_manager.h`
- `camera_yuv_query.h`
- `camera_color.h`

`camera_manager_cfg.h` is a private header and is included from the component implementation through `PRIV_INCLUDE_DIRS`.

## Notes

The component is designed around a single shared camera stream. Consumers should register callbacks or use the YUV query API rather than independently calling `esp_camera_fb_get()` for the same camera instance.
