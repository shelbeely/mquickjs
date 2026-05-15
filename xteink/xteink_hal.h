/*
 * Xteink X4 Hardware Abstraction Layer — public interface
 *
 * This header declares:
 *   1. HAL init/teardown
 *   2. Platform-level draw/input/FS/system/wifi/power functions
 *   3. MQuickJS JS-binding functions (JSCFunction signatures) that
 *      the stdlib table resolves to at run-time
 *
 * All hal_* functions are implemented in xteink_hal.c (ESP-IDF target).
 * All js_*  functions are implemented in xteink_hal.c and are called
 * directly by the MQuickJS VM through the generated stdlib table.
 *
 * Copyright (c) 2017-2025 Fabrice Bellard
 * Copyright (c) 2017-2025 Charlie Gordon
 * Xteink X4 additions: see LICENSE
 */
#ifndef XTEINK_HAL_H
#define XTEINK_HAL_H

#include <stdint.h>
#include <stddef.h>
#include "../mquickjs.h"
#include "xteink_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================
 * HAL init / teardown
 * ====================================================================== */

/* Initialise all peripherals (display, GPIO, SD card, WiFi stack).
 * Must be called once from app_main() before creating a JS context. */
void hal_init(void);

/* ======================================================================
 * Display HAL
 * ====================================================================== */

/* Fill the display with white and reset the draw color to black. */
void hal_display_clear(void);

/* Set the current draw color (0=black, 1=dark gray, 2=light gray, 3=white). */
void hal_display_set_color(int color);

/* Draw a UTF-8 string at pixel position (x, y) using the given font size. */
void hal_display_draw_text(int x, int y, const char *text, int font_size);

/* Draw an unfilled rectangle outline. */
void hal_display_draw_rect(int x, int y, int w, int h);

/* Draw a filled rectangle. */
void hal_display_fill_rect(int x, int y, int w, int h);

/* Draw a straight line from (x1,y1) to (x2,y2). */
void hal_display_draw_line(int x1, int y1, int x2, int y2);

/* Render a 1-bit-per-pixel bitmap from a byte array.
 * stride = bytes per row = (w + 7) / 8. */
void hal_display_draw_bitmap(int x, int y,
                              const uint8_t *data, size_t data_len,
                              int w, int h);

/* Trigger a full e-ink panel refresh (slow, no flash artifacts). */
void hal_display_refresh(void);

/* Trigger a partial e-ink panel refresh (faster, for small regions). */
void hal_display_partial_refresh(int x, int y, int w, int h);

/* ======================================================================
 * Input HAL
 * ====================================================================== */

/* Block until a button is pressed; return XTEINK_KEY_* constant. */
int hal_input_wait_key(void);

/* Non-blocking: return 1 if the given XTEINK_KEY_* is currently held. */
int hal_input_is_pressed(int key);

/* ======================================================================
 * System HAL
 * ====================================================================== */

/* Milliseconds since boot (wraps around after ~49 days). */
uint32_t hal_system_millis(void);

/* Busy-wait / yield for the given number of milliseconds. */
void hal_system_sleep(uint32_t ms);

/* Battery percentage 0–100. */
int hal_system_battery(void);

/* Software reboot. */
void hal_system_reboot(void);

/* ======================================================================
 * WiFi HAL
 * ====================================================================== */

/* Connect to the given SSID/password.  timeout_ms = 0 means use default.
 * Returns 1 on success, 0 on failure. */
int hal_wifi_connect(const char *ssid, const char *password,
                     uint32_t timeout_ms);

void hal_wifi_disconnect(void);
int  hal_wifi_is_connected(void);

/* Write the current IP address as a null-terminated string into buf
 * (buf must be at least 16 bytes). */
void hal_wifi_ip_address(char *buf, size_t buf_size);

/* ======================================================================
 * MQuickJS JS binding functions
 * (Implementations in xteink_hal.c; declared here for the stdlib builder)
 * ====================================================================== */

/* Display */
JSValue js_display_clear(JSContext *ctx, JSValue *this_val,
                          int argc, JSValue *argv);
JSValue js_display_set_color(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv);
JSValue js_display_draw_text(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv);
JSValue js_display_draw_rect(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv);
JSValue js_display_fill_rect(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv);
JSValue js_display_draw_line(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv);
JSValue js_display_draw_bitmap(JSContext *ctx, JSValue *this_val,
                                int argc, JSValue *argv);
JSValue js_display_refresh(JSContext *ctx, JSValue *this_val,
                            int argc, JSValue *argv);
JSValue js_display_partial_refresh(JSContext *ctx, JSValue *this_val,
                                    int argc, JSValue *argv);

/* Input */
JSValue js_input_wait_key(JSContext *ctx, JSValue *this_val,
                           int argc, JSValue *argv);
JSValue js_input_is_pressed(JSContext *ctx, JSValue *this_val,
                             int argc, JSValue *argv);

/* FS */
JSValue js_fs_open(JSContext *ctx, JSValue *this_val,
                   int argc, JSValue *argv);
JSValue js_fs_close(JSContext *ctx, JSValue *this_val,
                    int argc, JSValue *argv);
JSValue js_fs_read(JSContext *ctx, JSValue *this_val,
                   int argc, JSValue *argv);
JSValue js_fs_read_chunk(JSContext *ctx, JSValue *this_val,
                          int argc, JSValue *argv);
JSValue js_fs_write(JSContext *ctx, JSValue *this_val,
                    int argc, JSValue *argv);
JSValue js_fs_seek(JSContext *ctx, JSValue *this_val,
                   int argc, JSValue *argv);
JSValue js_fs_size(JSContext *ctx, JSValue *this_val,
                   int argc, JSValue *argv);
JSValue js_fs_exists(JSContext *ctx, JSValue *this_val,
                     int argc, JSValue *argv);
JSValue js_fs_list(JSContext *ctx, JSValue *this_val,
                   int argc, JSValue *argv);
JSValue js_fs_remove(JSContext *ctx, JSValue *this_val,
                     int argc, JSValue *argv);
JSValue js_fs_mkdir(JSContext *ctx, JSValue *this_val,
                    int argc, JSValue *argv);

/* System */
JSValue js_system_millis(JSContext *ctx, JSValue *this_val,
                          int argc, JSValue *argv);
JSValue js_system_sleep(JSContext *ctx, JSValue *this_val,
                         int argc, JSValue *argv);
JSValue js_system_battery(JSContext *ctx, JSValue *this_val,
                           int argc, JSValue *argv);
JSValue js_system_reboot(JSContext *ctx, JSValue *this_val,
                          int argc, JSValue *argv);

/* WiFi */
JSValue js_wifi_connect(JSContext *ctx, JSValue *this_val,
                         int argc, JSValue *argv);
JSValue js_wifi_disconnect(JSContext *ctx, JSValue *this_val,
                            int argc, JSValue *argv);
JSValue js_wifi_is_connected(JSContext *ctx, JSValue *this_val,
                              int argc, JSValue *argv);
JSValue js_wifi_ip_address(JSContext *ctx, JSValue *this_val,
                            int argc, JSValue *argv);

/* HTTP */
JSValue js_http_get(JSContext *ctx, JSValue *this_val,
                    int argc, JSValue *argv);
JSValue js_http_post(JSContext *ctx, JSValue *this_val,
                     int argc, JSValue *argv);

/* gc / load — forwarded from the base stdlib */
JSValue js_gc(JSContext *ctx, JSValue *this_val,
               int argc, JSValue *argv);
JSValue js_load(JSContext *ctx, JSValue *this_val,
                int argc, JSValue *argv);

#ifdef __cplusplus
}
#endif

#endif /* XTEINK_HAL_H */
