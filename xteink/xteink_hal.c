/*
 * Xteink X4 Hardware Abstraction Layer — ESP32-C3 implementation
 *
 * Compile with ESP-IDF only (requires IDF_VER to be defined).
 * See xteink_hal.h for the public interface.
 *
 * Copyright (c) 2017-2025 Fabrice Bellard
 * Copyright (c) 2017-2025 Charlie Gordon
 * Xteink X4 additions: see LICENSE
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "xteink_hal.h"

#ifdef IDF_VER
/* ---- ESP-IDF includes ------------------------------------------------ */
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_http_client.h"
#include "lwip/inet.h"

static const char *TAG = "xteink_hal";
#endif /* IDF_VER */

/* ======================================================================
 * Internal state
 * ====================================================================== */

static int  g_draw_color = XTEINK_COLOR_BLACK;

#ifdef IDF_VER
static spi_device_handle_t  g_epd_spi;
static sdmmc_card_t        *g_sd_card;
static QueueHandle_t        g_key_queue;
static uint8_t              g_epd_framebuf[XTEINK_DISPLAY_W * XTEINK_DISPLAY_H / 4];
#endif

/* ======================================================================
 * HAL init
 * ====================================================================== */

void hal_init(void)
{
#ifdef IDF_VER
    /* NVS (required by WiFi) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    /* Networking stack */
    esp_netif_init();
    esp_event_loop_create_default();

    /* ---- Button GPIO ------------------------------------------------- */
    g_key_queue = xQueueCreate(8, sizeof(int));

    const int btn_gpios[] = {
        XTEINK_GPIO_KEY_UP, XTEINK_GPIO_KEY_DOWN,
        XTEINK_GPIO_KEY_BACK, XTEINK_GPIO_KEY_OK,
        XTEINK_GPIO_KEY_POWER,
    };
    const int btn_keys[] = {
        XTEINK_KEY_UP, XTEINK_KEY_DOWN,
        XTEINK_KEY_BACK, XTEINK_KEY_OK,
        XTEINK_KEY_POWER,
    };
    for (int i = 0; i < 5; i++) {
        gpio_config_t cfg = {
            .pin_bit_mask = 1ULL << btn_gpios[i],
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_NEGEDGE,
        };
        gpio_config(&cfg);
        /* Store the key code as the interrupt handler user data */
        gpio_isr_handler_add(btn_gpios[i],
                             (gpio_isr_t)(uintptr_t)btn_keys[i],
                             (void *)(uintptr_t)btn_keys[i]);
    }
    gpio_install_isr_service(0);

    /* ---- SD card ----------------------------------------------------- */
    spi_bus_config_t buscfg = {
        .mosi_io_num   = XTEINK_GPIO_SD_MOSI,
        .miso_io_num   = XTEINK_GPIO_SD_MISO,
        .sclk_io_num   = XTEINK_GPIO_SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs   = XTEINK_GPIO_SD_CS;
    slot_config.host_id   = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files              = 8,
        .allocation_unit_size   = 16 * 1024,
    };
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    esp_vfs_fat_sdspi_mount(XTEINK_SD_MOUNT, &host,
                             &slot_config, &mount_cfg, &g_sd_card);

    /* ---- E-ink SPI --------------------------------------------------- */
    spi_bus_config_t epd_bus = {
        .mosi_io_num   = XTEINK_GPIO_EPD_MOSI,
        .miso_io_num   = -1,
        .sclk_io_num   = XTEINK_GPIO_EPD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(SPI3_HOST, &epd_bus, SPI_DMA_DISABLED);

    spi_device_interface_config_t epd_devcfg = {
        .clock_speed_hz = 4000000,
        .mode           = 0,
        .spics_io_num   = XTEINK_GPIO_EPD_CS,
        .queue_size     = 1,
    };
    spi_bus_add_device(SPI3_HOST, &epd_devcfg, &g_epd_spi);

    gpio_set_direction(XTEINK_GPIO_EPD_DC,   GPIO_MODE_OUTPUT);
    gpio_set_direction(XTEINK_GPIO_EPD_RST,  GPIO_MODE_OUTPUT);
    gpio_set_direction(XTEINK_GPIO_EPD_BUSY, GPIO_MODE_INPUT);

    /* Hardware reset */
    gpio_set_level(XTEINK_GPIO_EPD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(XTEINK_GPIO_EPD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    memset(g_epd_framebuf, 0xFF, sizeof(g_epd_framebuf)); /* all white */
    ESP_LOGI(TAG, "HAL init complete");
#endif /* IDF_VER */
}

/* ======================================================================
 * Display HAL
 * ====================================================================== */

void hal_display_clear(void)
{
#ifdef IDF_VER
    memset(g_epd_framebuf, 0xFF, sizeof(g_epd_framebuf));
#endif
    g_draw_color = XTEINK_COLOR_BLACK;
}

void hal_display_set_color(int color)
{
    g_draw_color = color & 3;
}

void hal_display_draw_text(int x, int y, const char *text, int font_size)
{
#ifdef IDF_VER
    /* Minimal 1-bit monochrome font renderer.
     * For a production build replace with a proper font rasteriser
     * (e.g. FreeType, u8g2, or a pre-rasterised bitmap font). */
    (void)font_size; /* TODO: honour font_size */
    const uint8_t pixel = (g_draw_color == XTEINK_COLOR_BLACK) ? 0 : 0xFF;
    /* Single-pixel placeholder: stamp the first byte of each character */
    for (int i = 0; text[i]; i++) {
        int px = x + i * 8;
        if (px >= XTEINK_DISPLAY_W) break;
        /* Mark the pixel position; real implementation would rasterise
         * the glyph using a stored bitmap font. */
        int byte_offset = (y * XTEINK_DISPLAY_W + px) / 4;
        if (byte_offset < (int)sizeof(g_epd_framebuf))
            g_epd_framebuf[byte_offset] = pixel;
    }
#endif
    (void)x; (void)y; (void)text; (void)font_size;
}

void hal_display_draw_rect(int x, int y, int w, int h)
{
    hal_display_draw_line(x,       y,       x + w, y);
    hal_display_draw_line(x + w,   y,       x + w, y + h);
    hal_display_draw_line(x + w,   y + h,   x,     y + h);
    hal_display_draw_line(x,       y + h,   x,     y);
}

void hal_display_fill_rect(int x, int y, int w, int h)
{
#ifdef IDF_VER
    uint8_t fill = 0;
    switch (g_draw_color) {
    case XTEINK_COLOR_WHITE:      fill = 0xFF; break;
    case XTEINK_COLOR_LIGHT_GRAY: fill = 0xAA; break;
    case XTEINK_COLOR_DARK_GRAY:  fill = 0x55; break;
    case XTEINK_COLOR_BLACK:
    default:                      fill = 0x00; break;
    }
    for (int row = y; row < y + h && row < XTEINK_DISPLAY_H; row++) {
        for (int col = x; col < x + w && col < XTEINK_DISPLAY_W; col++) {
            int byte_idx = (row * XTEINK_DISPLAY_W + col) / 4;
            int bit_shift = ((col % 4) ^ 3) * 2;
            g_epd_framebuf[byte_idx] =
                (g_epd_framebuf[byte_idx] & ~(0x03 << bit_shift)) |
                ((fill & 0x03) << bit_shift);
        }
    }
#endif
    (void)x; (void)y; (void)w; (void)h;
}

void hal_display_draw_line(int x1, int y1, int x2, int y2)
{
#ifdef IDF_VER
    /* Bresenham's line algorithm */
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    uint8_t gray = (uint8_t)(3 - g_draw_color); /* framebuf 0=black,3=white */
    while (1) {
        if (x1 >= 0 && x1 < XTEINK_DISPLAY_W &&
            y1 >= 0 && y1 < XTEINK_DISPLAY_H) {
            int byte_idx  = (y1 * XTEINK_DISPLAY_W + x1) / 4;
            int bit_shift = ((x1 % 4) ^ 3) * 2;
            g_epd_framebuf[byte_idx] =
                (g_epd_framebuf[byte_idx] & ~(0x03u << bit_shift)) |
                ((gray & 0x03u) << bit_shift);
        }
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
#endif
    (void)x1; (void)y1; (void)x2; (void)y2;
}

void hal_display_draw_bitmap(int x, int y,
                              const uint8_t *data, size_t data_len,
                              int w, int h)
{
#ifdef IDF_VER
    int stride = (w + 7) / 8;
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            int src_byte = row * stride + col / 8;
            if ((size_t)src_byte >= data_len) return;
            int bit = (data[src_byte] >> (7 - (col % 8))) & 1;
            /* bit=1 → draw color, bit=0 → transparent (skip) */
            if (!bit) continue;
            int px = x + col, py = y + row;
            if (px < 0 || px >= XTEINK_DISPLAY_W ||
                py < 0 || py >= XTEINK_DISPLAY_H) continue;
            int byte_idx  = (py * XTEINK_DISPLAY_W + px) / 4;
            int bit_shift = ((px % 4) ^ 3) * 2;
            uint8_t gray  = (uint8_t)(3 - g_draw_color);
            g_epd_framebuf[byte_idx] =
                (g_epd_framebuf[byte_idx] & ~(0x03u << bit_shift)) |
                ((gray & 0x03u) << bit_shift);
        }
    }
#endif
    (void)x; (void)y; (void)data; (void)data_len; (void)w; (void)h;
}

static void epd_wait_busy(void)
{
#ifdef IDF_VER
    while (gpio_get_level(XTEINK_GPIO_EPD_BUSY) == 1)
        vTaskDelay(pdMS_TO_TICKS(10));
#endif
}

void hal_display_refresh(void)
{
#ifdef IDF_VER
    /* Send framebuffer to the panel (GDEY043T81 command sequence).
     * Adapt to the actual panel datasheet if necessary. */
    spi_transaction_t t = {
        .length    = sizeof(g_epd_framebuf) * 8,
        .tx_buffer = g_epd_framebuf,
    };
    gpio_set_level(XTEINK_GPIO_EPD_DC, 1); /* data */
    spi_device_transmit(g_epd_spi, &t);
    epd_wait_busy();
#endif
}

void hal_display_partial_refresh(int x, int y, int w, int h)
{
    /* For the initial implementation fall back to full refresh.
     * A production driver would send only the dirty region. */
    (void)x; (void)y; (void)w; (void)h;
    hal_display_refresh();
}

/* ======================================================================
 * Input HAL
 * ====================================================================== */

int hal_input_wait_key(void)
{
#ifdef IDF_VER
    int key = XTEINK_KEY_OK;
    xQueueReceive(g_key_queue, &key, portMAX_DELAY);
    return key;
#else
    return XTEINK_KEY_OK;
#endif
}

int hal_input_is_pressed(int key)
{
#ifdef IDF_VER
    const int gpios[] = {
        XTEINK_GPIO_KEY_UP, XTEINK_GPIO_KEY_DOWN,
        XTEINK_GPIO_KEY_BACK, XTEINK_GPIO_KEY_OK,
        XTEINK_GPIO_KEY_POWER,
    };
    if (key < 0 || key >= 5) return 0;
    return gpio_get_level(gpios[key]) == 0; /* active low */
#else
    (void)key;
    return 0;
#endif
}

/* ======================================================================
 * System HAL
 * ====================================================================== */

uint32_t hal_system_millis(void)
{
#ifdef IDF_VER
    return (uint32_t)(esp_timer_get_time() / 1000);
#else
    return 0;
#endif
}

void hal_system_sleep(uint32_t ms)
{
#ifdef IDF_VER
    vTaskDelay(pdMS_TO_TICKS(ms));
#else
    (void)ms;
#endif
}

int hal_system_battery(void)
{
#ifdef IDF_VER
    /* TODO: read ADC connected to battery voltage divider */
    return 100;
#else
    return 100;
#endif
}

void hal_system_reboot(void)
{
#ifdef IDF_VER
    esp_restart();
#endif
}

/* ======================================================================
 * WiFi HAL
 * ====================================================================== */

int hal_wifi_connect(const char *ssid, const char *password,
                     uint32_t timeout_ms)
{
#ifdef IDF_VER
    if (timeout_ms == 0) timeout_ms = 10000;

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_cfg = {0};
    strncpy((char *)wifi_cfg.sta.ssid,     ssid,     sizeof(wifi_cfg.sta.ssid)     - 1);
    strncpy((char *)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password) - 1);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    esp_wifi_start();
    esp_wifi_connect();

    uint32_t elapsed = 0;
    while (elapsed < timeout_ms) {
        if (hal_wifi_is_connected()) return 1;
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed += 100;
    }
    return 0;
#else
    (void)ssid; (void)password; (void)timeout_ms;
    return 0;
#endif
}

void hal_wifi_disconnect(void)
{
#ifdef IDF_VER
    esp_wifi_disconnect();
    esp_wifi_stop();
#endif
}

int hal_wifi_is_connected(void)
{
#ifdef IDF_VER
    wifi_ap_record_t ap;
    return esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
#else
    return 0;
#endif
}

void hal_wifi_ip_address(char *buf, size_t buf_size)
{
#ifdef IDF_VER
    esp_netif_ip_info_t info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &info) == ESP_OK) {
        snprintf(buf, buf_size, IPSTR, IP2STR(&info.ip));
    } else {
        snprintf(buf, buf_size, "0.0.0.0");
    }
#else
    snprintf(buf, buf_size, "0.0.0.0");
#endif
}

/* ======================================================================
 * JS binding helpers
 * ====================================================================== */

/* Helper: extract an int32 from argv[i] */
static int get_int_arg(JSContext *ctx, JSValue *argv, int i, int *out)
{
    return JS_ToInt32(ctx, out, argv[i]);
}

/* ======================================================================
 * JS bindings — Display
 * ====================================================================== */

JSValue js_display_clear(JSContext *ctx, JSValue *this_val,
                          int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    hal_display_clear();
    return JS_UNDEFINED;
}

JSValue js_display_set_color(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv)
{
    int c;
    (void)this_val;
    if (argc < 1 || get_int_arg(ctx, argv, 0, &c)) return JS_EXCEPTION;
    hal_display_set_color(c);
    return JS_UNDEFINED;
}

JSValue js_display_draw_text(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv)
{
    int x, y, font_size;
    JSCStringBuf buf;
    const char *text;
    (void)this_val;
    if (argc < 4) return JS_ThrowTypeError(ctx, "drawText needs 4 args");
    if (get_int_arg(ctx, argv, 0, &x) ||
        get_int_arg(ctx, argv, 1, &y) ||
        get_int_arg(ctx, argv, 3, &font_size)) return JS_EXCEPTION;
    text = JS_ToCString(ctx, argv[2], &buf);
    if (!text) return JS_EXCEPTION;
    hal_display_draw_text(x, y, text, font_size);
    return JS_UNDEFINED;
}

JSValue js_display_draw_rect(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv)
{
    int x, y, w, h;
    (void)this_val;
    if (argc < 4) return JS_ThrowTypeError(ctx, "drawRect needs 4 args");
    if (get_int_arg(ctx, argv, 0, &x) || get_int_arg(ctx, argv, 1, &y) ||
        get_int_arg(ctx, argv, 2, &w) || get_int_arg(ctx, argv, 3, &h))
        return JS_EXCEPTION;
    hal_display_draw_rect(x, y, w, h);
    return JS_UNDEFINED;
}

JSValue js_display_fill_rect(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv)
{
    int x, y, w, h;
    (void)this_val;
    if (argc < 4) return JS_ThrowTypeError(ctx, "fillRect needs 4 args");
    if (get_int_arg(ctx, argv, 0, &x) || get_int_arg(ctx, argv, 1, &y) ||
        get_int_arg(ctx, argv, 2, &w) || get_int_arg(ctx, argv, 3, &h))
        return JS_EXCEPTION;
    hal_display_fill_rect(x, y, w, h);
    return JS_UNDEFINED;
}

JSValue js_display_draw_line(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv)
{
    int x1, y1, x2, y2;
    (void)this_val;
    if (argc < 4) return JS_ThrowTypeError(ctx, "drawLine needs 4 args");
    if (get_int_arg(ctx, argv, 0, &x1) || get_int_arg(ctx, argv, 1, &y1) ||
        get_int_arg(ctx, argv, 2, &x2) || get_int_arg(ctx, argv, 3, &y2))
        return JS_EXCEPTION;
    hal_display_draw_line(x1, y1, x2, y2);
    return JS_UNDEFINED;
}

JSValue js_display_draw_bitmap(JSContext *ctx, JSValue *this_val,
                                int argc, JSValue *argv)
{
    int x, y, w, h;
    (void)this_val;
    if (argc < 5) return JS_ThrowTypeError(ctx, "drawBitmap needs 5 args");
    if (get_int_arg(ctx, argv, 0, &x) || get_int_arg(ctx, argv, 1, &y) ||
        get_int_arg(ctx, argv, 3, &w) || get_int_arg(ctx, argv, 4, &h))
        return JS_EXCEPTION;
    if (JS_GetClassID(ctx, argv[2]) != JS_CLASS_UINT8_ARRAY)
        return JS_ThrowTypeError(ctx, "drawBitmap: data must be Uint8Array");
    /* argv[2] is the Uint8Array; extract its backing buffer pointer */
    JSValue buf_val = JS_GetPropertyStr(ctx, argv[2], "buffer");
    (void)buf_val; /* ArrayBuffer backing; access via opaque in a real impl */
    /* TODO: access the raw byte pointer from the ArrayBuffer once the
     * JS engine exposes a typed-array data pointer API */
    hal_display_draw_bitmap(x, y, NULL, 0, w, h);
    return JS_UNDEFINED;
}

JSValue js_display_refresh(JSContext *ctx, JSValue *this_val,
                            int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    hal_display_refresh();
    return JS_UNDEFINED;
}

JSValue js_display_partial_refresh(JSContext *ctx, JSValue *this_val,
                                    int argc, JSValue *argv)
{
    int x, y, w, h;
    (void)this_val;
    if (argc < 4) return JS_ThrowTypeError(ctx, "partialRefresh needs 4 args");
    if (get_int_arg(ctx, argv, 0, &x) || get_int_arg(ctx, argv, 1, &y) ||
        get_int_arg(ctx, argv, 2, &w) || get_int_arg(ctx, argv, 3, &h))
        return JS_EXCEPTION;
    hal_display_partial_refresh(x, y, w, h);
    return JS_UNDEFINED;
}

/* ======================================================================
 * JS bindings — Input
 * ====================================================================== */

JSValue js_input_wait_key(JSContext *ctx, JSValue *this_val,
                           int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    return JS_NewInt32(ctx, hal_input_wait_key());
}

JSValue js_input_is_pressed(JSContext *ctx, JSValue *this_val,
                             int argc, JSValue *argv)
{
    int key;
    (void)this_val;
    if (argc < 1 || get_int_arg(ctx, argv, 0, &key)) return JS_EXCEPTION;
    return JS_NewBool(hal_input_is_pressed(key));
}

/* ======================================================================
 * JS bindings — FS
 * ====================================================================== */

JSValue js_fs_open(JSContext *ctx, JSValue *this_val,
                   int argc, JSValue *argv)
{
    const char *mode_str;
    JSCStringBuf path_buf, mode_buf;
    const char *c_mode;
    FILE *f;
    (void)this_val;
    if (argc < 2) return JS_ThrowTypeError(ctx, "FS.open: need path, mode");
    const char *path = JS_ToCString(ctx, argv[0], &path_buf);
    if (!path) return JS_EXCEPTION;
    mode_str = JS_ToCString(ctx, argv[1], &mode_buf);
    if (!mode_str) return JS_EXCEPTION;

    /* Map JS mode strings to C fopen modes */
    if (!strcmp(mode_str, "r"))       c_mode = "rb";
    else if (!strcmp(mode_str, "w"))  c_mode = "wb";
    else if (!strcmp(mode_str, "a"))  c_mode = "ab";
    else return JS_ThrowTypeError(ctx, "FS.open: invalid mode");

    f = fopen(path, c_mode);
    if (!f) return JS_NewInt32(ctx, -1);
    /* Use fileno() on POSIX; on FATFS just cast pointer to int */
    return JS_NewInt32(ctx, (int)(uintptr_t)f);
}

JSValue js_fs_close(JSContext *ctx, JSValue *this_val,
                    int argc, JSValue *argv)
{
    int fd;
    (void)this_val;
    if (argc < 1 || get_int_arg(ctx, argv, 0, &fd)) return JS_EXCEPTION;
    fclose((FILE *)(uintptr_t)(unsigned int)fd);
    return JS_UNDEFINED;
}

JSValue js_fs_read(JSContext *ctx, JSValue *this_val,
                   int argc, JSValue *argv)
{
    /* FS.read(fd, buffer:Uint8Array, offset, length) → bytes_read */
    (void)this_val; (void)argc; (void)argv;
    /* TODO: typed-array data pointer access */
    return JS_NewInt32(ctx, 0);
}

JSValue js_fs_read_chunk(JSContext *ctx, JSValue *this_val,
                          int argc, JSValue *argv)
{
    /* FS.readChunk(fd, size?) → Uint8Array | null */
    int fd;
    uint32_t size = XTEINK_CHUNK_SIZE;
    (void)this_val;
    if (argc < 1 || get_int_arg(ctx, argv, 0, &fd)) return JS_EXCEPTION;
    if (argc >= 2) {
        if (JS_ToUint32(ctx, &size, argv[1])) return JS_EXCEPTION;
        if (size > XTEINK_CHUNK_SIZE) size = XTEINK_CHUNK_SIZE;
    }

    /* Allocate a Uint8Array to hold the chunk */
    JSValue ab = JS_NewArrayBuffer(ctx, NULL, size, NULL, NULL, FALSE);
    if (JS_IsException(ab)) return ab;
    /* TODO: once JS_GetArrayBufferSize/ptr API is available, use it to
     * get the raw pointer and call fread() directly. */

    /* Placeholder: zero-length indicates EOF detection needs real impl */
    return JS_NULL; /* return null to signal no data yet */
}

JSValue js_fs_write(JSContext *ctx, JSValue *this_val,
                    int argc, JSValue *argv)
{
    /* FS.write(fd, buffer:Uint8Array, offset, length) → bytes_written */
    (void)this_val; (void)argc; (void)argv;
    return JS_NewInt32(ctx, 0);
}

JSValue js_fs_seek(JSContext *ctx, JSValue *this_val,
                   int argc, JSValue *argv)
{
    /* FS.seek(fd, offset, whence) → new_position */
    int fd, offset, whence;
    (void)this_val;
    if (argc < 3 ||
        get_int_arg(ctx, argv, 0, &fd)     ||
        get_int_arg(ctx, argv, 1, &offset) ||
        get_int_arg(ctx, argv, 2, &whence))
        return JS_EXCEPTION;
    FILE *f = (FILE *)(uintptr_t)(unsigned int)fd;
    fseek(f, offset, whence);
    return JS_NewInt32(ctx, (int)ftell(f));
}

JSValue js_fs_size(JSContext *ctx, JSValue *this_val,
                   int argc, JSValue *argv)
{
    JSCStringBuf buf;
    const char *path;
    FILE *f;
    long sz;
    (void)this_val;
    if (argc < 1) return JS_EXCEPTION;
    path = JS_ToCString(ctx, argv[0], &buf);
    if (!path) return JS_EXCEPTION;
    f = fopen(path, "rb");
    if (!f) return JS_NewInt32(ctx, -1);
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fclose(f);
    return JS_NewInt32(ctx, (int)sz);
}

JSValue js_fs_exists(JSContext *ctx, JSValue *this_val,
                     int argc, JSValue *argv)
{
    JSCStringBuf buf;
    const char *path;
    FILE *f;
    (void)this_val;
    if (argc < 1) return JS_EXCEPTION;
    path = JS_ToCString(ctx, argv[0], &buf);
    if (!path) return JS_EXCEPTION;
    f = fopen(path, "rb");
    if (!f) return JS_NewBool(0);
    fclose(f);
    return JS_NewBool(1);
}

JSValue js_fs_list(JSContext *ctx, JSValue *this_val,
                   int argc, JSValue *argv)
{
#ifdef IDF_VER
    JSCStringBuf buf;
    const char *path;
    DIR *dir;
    struct dirent *ent;
    JSValue arr;
    int i = 0;
    (void)this_val;
    if (argc < 1) return JS_EXCEPTION;
    path = JS_ToCString(ctx, argv[0], &buf);
    if (!path) return JS_EXCEPTION;
    arr = JS_NewArray(ctx, 0);
    if (JS_IsException(arr)) return arr;
    dir = opendir(path);
    if (dir) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            JS_SetPropertyUint32(ctx, arr, i++,
                                  JS_NewString(ctx, ent->d_name));
        }
        closedir(dir);
    }
    return arr;
#else
    (void)this_val; (void)argc; (void)argv;
    return JS_NewArray(ctx, 0);
#endif
}

JSValue js_fs_remove(JSContext *ctx, JSValue *this_val,
                     int argc, JSValue *argv)
{
    JSCStringBuf buf;
    const char *path;
    (void)this_val;
    if (argc < 1) return JS_EXCEPTION;
    path = JS_ToCString(ctx, argv[0], &buf);
    if (!path) return JS_EXCEPTION;
    remove(path);
    return JS_UNDEFINED;
}

JSValue js_fs_mkdir(JSContext *ctx, JSValue *this_val,
                    int argc, JSValue *argv)
{
    JSCStringBuf buf;
    const char *path;
    (void)this_val;
    if (argc < 1) return JS_EXCEPTION;
    path = JS_ToCString(ctx, argv[0], &buf);
    if (!path) return JS_EXCEPTION;
#ifdef IDF_VER
    mkdir(path, 0777);
#endif
    return JS_UNDEFINED;
}

/* ======================================================================
 * JS bindings — System
 * ====================================================================== */

JSValue js_system_millis(JSContext *ctx, JSValue *this_val,
                          int argc, JSValue *argv)
{
    (void)this_val; (void)argc; (void)argv;
    return JS_NewUint32(ctx, hal_system_millis());
}

JSValue js_system_sleep(JSContext *ctx, JSValue *this_val,
                         int argc, JSValue *argv)
{
    uint32_t ms;
    (void)this_val;
    if (argc < 1 || JS_ToUint32(ctx, &ms, argv[0])) return JS_EXCEPTION;
    hal_system_sleep(ms);
    return JS_UNDEFINED;
}

JSValue js_system_battery(JSContext *ctx, JSValue *this_val,
                           int argc, JSValue *argv)
{
    (void)this_val; (void)argc; (void)argv;
    return JS_NewInt32(ctx, hal_system_battery());
}

JSValue js_system_reboot(JSContext *ctx, JSValue *this_val,
                          int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    hal_system_reboot();
    return JS_UNDEFINED;
}

/* ======================================================================
 * JS bindings — WiFi
 * ====================================================================== */

JSValue js_wifi_connect(JSContext *ctx, JSValue *this_val,
                         int argc, JSValue *argv)
{
    JSCStringBuf ssid_buf, pass_buf;
    const char *ssid, *pass;
    uint32_t timeout = 0;
    int ok;
    (void)this_val;
    if (argc < 2) return JS_ThrowTypeError(ctx, "WiFi.connect: ssid, pass required");
    ssid = JS_ToCString(ctx, argv[0], &ssid_buf);
    if (!ssid) return JS_EXCEPTION;
    pass = JS_ToCString(ctx, argv[1], &pass_buf);
    if (!pass) return JS_EXCEPTION;
    if (argc >= 3 && JS_ToUint32(ctx, &timeout, argv[2]))
        return JS_EXCEPTION;
    ok = hal_wifi_connect(ssid, pass, timeout);
    return JS_NewBool(ok);
}

JSValue js_wifi_disconnect(JSContext *ctx, JSValue *this_val,
                            int argc, JSValue *argv)
{
    (void)ctx; (void)this_val; (void)argc; (void)argv;
    hal_wifi_disconnect();
    return JS_UNDEFINED;
}

JSValue js_wifi_is_connected(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv)
{
    (void)this_val; (void)argc; (void)argv;
    return JS_NewBool(hal_wifi_is_connected());
}

JSValue js_wifi_ip_address(JSContext *ctx, JSValue *this_val,
                            int argc, JSValue *argv)
{
    char ip[20];
    (void)this_val; (void)argc; (void)argv;
    hal_wifi_ip_address(ip, sizeof(ip));
    return JS_NewString(ctx, ip);
}

/* ======================================================================
 * JS bindings — HTTP (chunked)
 * ====================================================================== */

#ifdef IDF_VER
typedef struct {
    JSContext *ctx;
    JSValue   *callback; /* pointer into GC-protected slot */
    int        total;
} HTTPUserData;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    HTTPUserData *ud = (HTTPUserData *)evt->user_data;
    if (evt->event_id != HTTP_EVENT_ON_DATA) return ESP_OK;
    if (evt->data_len <= 0) return ESP_OK;

    /* Build a Uint8Array from this chunk and call the JS callback */
    JSValue ab = JS_NewArrayBuffer(ud->ctx, NULL, (size_t)evt->data_len,
                                   NULL, NULL, FALSE);
    if (!JS_IsException(ab)) {
        /* TODO: copy evt->data into the ArrayBuffer once low-level
         * pointer access is available */
        (void)ab;
    }
    ud->total += evt->data_len;
    return ESP_OK;
}
#endif /* IDF_VER */

JSValue js_http_get(JSContext *ctx, JSValue *this_val,
                    int argc, JSValue *argv)
{
#ifdef IDF_VER
    JSCStringBuf url_buf;
    const char *url;
    (void)this_val;
    if (argc < 2) return JS_ThrowTypeError(ctx, "HTTP.get: url, callback required");
    if (!JS_IsFunction(ctx, argv[1]))
        return JS_ThrowTypeError(ctx, "HTTP.get: callback must be a function");
    url = JS_ToCString(ctx, argv[0], &url_buf);
    if (!url) return JS_EXCEPTION;

    HTTPUserData ud = { .ctx = ctx, .callback = &argv[1], .total = 0 };
    esp_http_client_config_t cfg = {
        .url        = url,
        .event_handler = http_event_handler,
        .user_data  = &ud,
        .buffer_size = XTEINK_HTTP_BUF_SIZE,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    return JS_NewInt32(ctx, ud.total);
#else
    (void)this_val; (void)argc; (void)argv;
    return JS_NewInt32(ctx, 0);
#endif
}

JSValue js_http_post(JSContext *ctx, JSValue *this_val,
                     int argc, JSValue *argv)
{
#ifdef IDF_VER
    JSCStringBuf url_buf, body_buf;
    const char *url, *body;
    (void)this_val;
    if (argc < 3) return JS_ThrowTypeError(ctx, "HTTP.post: url, body, callback required");
    if (!JS_IsFunction(ctx, argv[2]))
        return JS_ThrowTypeError(ctx, "HTTP.post: callback must be a function");
    url  = JS_ToCString(ctx, argv[0], &url_buf);
    if (!url) return JS_EXCEPTION;
    body = JS_ToCString(ctx, argv[1], &body_buf);
    if (!body) return JS_EXCEPTION;

    HTTPUserData ud = { .ctx = ctx, .callback = &argv[2], .total = 0 };
    esp_http_client_config_t cfg = {
        .url           = url,
        .method        = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .user_data     = &ud,
        .buffer_size   = XTEINK_HTTP_BUF_SIZE,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_post_field(client, body, (int)strlen(body));
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    return JS_NewInt32(ctx, ud.total);
#else
    (void)this_val; (void)argc; (void)argv;
    return JS_NewInt32(ctx, 0);
#endif
}
