/*
 * MQuickJS for Xteink X4 — compile-time configuration
 *
 * Hardware: ESP32-C3 (RISC-V 32-bit, 160 MHz)
 * Display:  4.3" e-ink 480×800, 4-gray, Good Display GDEY043T81
 * Controls: 5 physical buttons (KEY_UP, KEY_DOWN, KEY_BACK, KEY_OK, KEY_POWER)
 * Storage:  FAT32 microSD card
 */
#ifndef XTEINK_CONFIG_H
#define XTEINK_CONFIG_H

/* ---- Memory limits -------------------------------------------------- */

/* Maximum JS working memory per app (64 KB) */
#define XTEINK_MEM_SIZE         (64u * 1024u)

/* Maximum size of a .js source file or .app bytecode file (32 KB) */
#define XTEINK_MAX_SCRIPT_SIZE  (32u * 1024u)

/* Recommended file read chunk size to stay within memory budget (4 KB) */
#define XTEINK_CHUNK_SIZE       4096u

/* ---- File system paths ----------------------------------------------- */

/* SD card VFS mount point (ESP-IDF esp_vfs_fat) */
#define XTEINK_SD_MOUNT         "/sdcard"

/* Directory scanned for user apps */
#define XTEINK_APP_DIR          "/sdcard/apps"

/* ---- Display --------------------------------------------------------- */

#define XTEINK_DISPLAY_W        480
#define XTEINK_DISPLAY_H        800

/* Gray level constants (0 = black … 3 = white) */
#define XTEINK_COLOR_BLACK      0
#define XTEINK_COLOR_DARK_GRAY  1
#define XTEINK_COLOR_LIGHT_GRAY 2
#define XTEINK_COLOR_WHITE      3

/* ---- Input key codes ------------------------------------------------- */

#define XTEINK_KEY_UP           0
#define XTEINK_KEY_DOWN         1
#define XTEINK_KEY_BACK         2
#define XTEINK_KEY_OK           3
#define XTEINK_KEY_POWER        4

/* ---- GPIO pin mapping (ESP32-C3) ------------------------------------ */
/*
 * Adjust these to match the actual PCB schematic.
 * Reference: https://github.com/sunwoods/Xteink-X4
 */
#define XTEINK_GPIO_KEY_UP      4
#define XTEINK_GPIO_KEY_DOWN    5
#define XTEINK_GPIO_KEY_BACK    6
#define XTEINK_GPIO_KEY_OK      7
#define XTEINK_GPIO_KEY_POWER   0   /* GPIO0 is also the BOOT button */

/* E-ink panel SPI */
#define XTEINK_GPIO_EPD_CS      10
#define XTEINK_GPIO_EPD_CLK     8
#define XTEINK_GPIO_EPD_MOSI    9
#define XTEINK_GPIO_EPD_DC      2
#define XTEINK_GPIO_EPD_RST     3
#define XTEINK_GPIO_EPD_BUSY    1

/* SD card SPI */
#define XTEINK_GPIO_SD_CS       20
#define XTEINK_GPIO_SD_CLK      19
#define XTEINK_GPIO_SD_MOSI     18
#define XTEINK_GPIO_SD_MISO     21

/* ---- Firmware version ----------------------------------------------- */

#define XTEINK_FIRMWARE_VERSION "v1.0.0"

/* ---- Launcher UI ----------------------------------------------------- */

/* Font sizes supported by the drawing layer */
#define XTEINK_FONT_SMALL       12
#define XTEINK_FONT_MEDIUM      16
#define XTEINK_FONT_LARGE       24

/* Number of app names shown per launcher page */
#define XTEINK_LAUNCHER_PAGE_SIZE  8

/* Maximum number of apps discoverable in the apps directory */
#define XTEINK_MAX_APPS        64

/* Maximum length of an app filename (basename only, including extension) */
#define XTEINK_APP_NAME_LEN    64

/* ---- Power management ----------------------------------------------- */

/* Idle time before entering ESP light-sleep (milliseconds) */
#define XTEINK_SLEEP_IDLE_MS   (30u * 1000u)

/* ---- HTTP client ----------------------------------------------------- */

/* Maximum URL length */
#define XTEINK_HTTP_URL_LEN    256

/* HTTP receive buffer (fed to the JS chunkCallback in pieces) */
#define XTEINK_HTTP_BUF_SIZE   XTEINK_CHUNK_SIZE

#endif /* XTEINK_CONFIG_H */
