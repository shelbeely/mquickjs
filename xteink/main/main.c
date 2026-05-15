/*
 * Xteink X4 firmware — ESP-IDF app_main entry point
 *
 * Copyright (c) 2017-2025 Fabrice Bellard
 * Copyright (c) 2017-2025 Charlie Gordon
 * Xteink X4 additions: see LICENSE
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "xteink_hal.h"
#include "xteink_loader.h"

static const char *TAG = "xteink_main";

void app_main(void)
{
    ESP_LOGI(TAG, "Xteink X4 firmware starting (MQuickJS %s)",
             XTEINK_FIRMWARE_VERSION);

    /* Initialise display, buttons, SD card, WiFi stack */
    hal_init();

    /* Run the app launcher (never returns) */
    xteink_run_launcher();
}
