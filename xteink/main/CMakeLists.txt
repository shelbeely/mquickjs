# MQuickJS for Xteink X4 — ESP-IDF component registration
#
# This CMakeLists.txt registers all MQuickJS engine sources plus the
# Xteink HAL/loader as a single ESP-IDF component.  The generated
# xteink_stdlib.h must exist before building (run `make xteink_stdlib.h`
# in the repo root on the host first).

idf_component_register(
    SRCS
        # MQuickJS engine (from repo root)
        "../../mquickjs.c"
        "../../dtoa.c"
        "../../libm.c"
        "../../cutils.c"
        # Xteink firmware layer
        "../xteink_hal.c"
        "../xteink_loader.c"
        # App entry point
        "main.c"
    INCLUDE_DIRS
        "../.."          # mquickjs.h, cutils.h
        ".."             # xteink_config.h, xteink_hal.h, xteink_loader.h
    REQUIRES
        driver
        fatfs
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        esp_http_client
        esp_timer
        spi_flash
)

# Compiler flags for the engine: size-optimised, no math exceptions
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Os
    -fno-math-errno
    -fno-trapping-math
    -DCONFIG_SMALL=1
)
