/*
 * Xteink X4 standard library — build tool (host-side)
 *
 * This file is compiled on the host machine by mquickjs_build to generate
 * xteink_stdlib.h, which is then included in the ESP32-C3 firmware build.
 *
 * It defines all hardware-facing JS objects:
 *   Display, Input, FS, System, WiFi, HTTP
 *
 * plus the full MQuickJS base library by including mqjs_stdlib.c with the
 * CONFIG_XTEINK flag, which replaces the desktop-only timer APIs with the
 * Xteink hardware objects.
 *
 * Usage (from the repo root):
 *   make xteink_stdlib.h
 *
 * The C function names referenced here (js_display_clear, js_fs_open, …)
 * are implemented in xteink_hal.c for the embedded target.
 *
 * Copyright (c) 2017-2025 Fabrice Bellard
 * Copyright (c) 2017-2025 Charlie Gordon
 * Xteink X4 additions: see LICENSE
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../mquickjs_build.h"
#include "xteink_config.h"

/* ---- Display --------------------------------------------------------- */

static const JSPropDef js_display_props[] = {
    JS_PROP_DOUBLE_DEF("width",  XTEINK_DISPLAY_W, 0),
    JS_PROP_DOUBLE_DEF("height", XTEINK_DISPLAY_H, 0),
    JS_CFUNC_DEF("clear",          0, js_display_clear),
    JS_CFUNC_DEF("setColor",       1, js_display_set_color),
    JS_CFUNC_DEF("drawText",       4, js_display_draw_text),
    JS_CFUNC_DEF("drawRect",       4, js_display_draw_rect),
    JS_CFUNC_DEF("fillRect",       4, js_display_fill_rect),
    JS_CFUNC_DEF("drawLine",       4, js_display_draw_line),
    JS_CFUNC_DEF("drawBitmap",     5, js_display_draw_bitmap),
    JS_CFUNC_DEF("refresh",        0, js_display_refresh),
    JS_CFUNC_DEF("partialRefresh", 4, js_display_partial_refresh),
    JS_PROP_END,
};

static const JSClassDef js_display_obj =
    JS_OBJECT_DEF("Display", js_display_props);

/* ---- Input ----------------------------------------------------------- */

static const JSPropDef js_input_props[] = {
    JS_PROP_DOUBLE_DEF("KEY_UP",    XTEINK_KEY_UP,    0),
    JS_PROP_DOUBLE_DEF("KEY_DOWN",  XTEINK_KEY_DOWN,  0),
    JS_PROP_DOUBLE_DEF("KEY_BACK",  XTEINK_KEY_BACK,  0),
    JS_PROP_DOUBLE_DEF("KEY_OK",    XTEINK_KEY_OK,    0),
    JS_PROP_DOUBLE_DEF("KEY_POWER", XTEINK_KEY_POWER, 0),
    JS_CFUNC_DEF("waitKey",   0, js_input_wait_key),
    JS_CFUNC_DEF("isPressed", 1, js_input_is_pressed),
    JS_PROP_END,
};

static const JSClassDef js_input_obj =
    JS_OBJECT_DEF("Input", js_input_props);

/* ---- FS -------------------------------------------------------------- */

static const JSPropDef js_fs_props[] = {
    JS_CFUNC_DEF("open",      2, js_fs_open),
    JS_CFUNC_DEF("close",     1, js_fs_close),
    JS_CFUNC_DEF("read",      4, js_fs_read),
    JS_CFUNC_DEF("readChunk", 2, js_fs_read_chunk),
    JS_CFUNC_DEF("write",     4, js_fs_write),
    JS_CFUNC_DEF("seek",      3, js_fs_seek),
    JS_CFUNC_DEF("size",      1, js_fs_size),
    JS_CFUNC_DEF("exists",    1, js_fs_exists),
    JS_CFUNC_DEF("list",      1, js_fs_list),
    JS_CFUNC_DEF("remove",    1, js_fs_remove),
    JS_CFUNC_DEF("mkdir",     1, js_fs_mkdir),
    JS_PROP_END,
};

static const JSClassDef js_fs_obj =
    JS_OBJECT_DEF("FS", js_fs_props);

/* ---- System ---------------------------------------------------------- */

static const JSPropDef js_system_props[] = {
    JS_PROP_STRING_DEF("version", XTEINK_FIRMWARE_VERSION, 0),
    JS_CFUNC_DEF("millis",  0, js_system_millis),
    JS_CFUNC_DEF("sleep",   1, js_system_sleep),
    JS_CFUNC_DEF("battery", 0, js_system_battery),
    JS_CFUNC_DEF("reboot",  0, js_system_reboot),
    JS_PROP_END,
};

static const JSClassDef js_system_obj =
    JS_OBJECT_DEF("System", js_system_props);

/* ---- WiFi ------------------------------------------------------------ */

static const JSPropDef js_wifi_props[] = {
    JS_CFUNC_DEF("connect",     3, js_wifi_connect),
    JS_CFUNC_DEF("disconnect",  0, js_wifi_disconnect),
    JS_CFUNC_DEF("isConnected", 0, js_wifi_is_connected),
    JS_CFUNC_DEF("ipAddress",   0, js_wifi_ip_address),
    JS_PROP_END,
};

static const JSClassDef js_wifi_obj =
    JS_OBJECT_DEF("WiFi", js_wifi_props);

/* ---- HTTP ------------------------------------------------------------ */

static const JSPropDef js_http_props[] = {
    JS_CFUNC_DEF("get",  2, js_http_get),
    JS_CFUNC_DEF("post", 3, js_http_post),
    JS_PROP_END,
};

static const JSClassDef js_http_obj =
    JS_OBJECT_DEF("HTTP", js_http_props);

/* ---- Pull in the full MQuickJS base stdlib with CONFIG_XTEINK -------- */

#define CONFIG_XTEINK
#include "../mqjs_stdlib.c"
