/*
 * Xteink X4 App Loader — public interface
 *
 * The loader:
 *   1. Scans SDCARD/apps/ for .js and .app files
 *   2. Draws a scrollable launcher menu on the e-ink display
 *   3. On user selection, creates a fresh MQuickJS context, runs
 *      the app, then cleans up
 *   4. Handles JS exceptions by displaying them and returning to
 *      the launcher
 *
 * Copyright (c) 2017-2025 Fabrice Bellard
 * Copyright (c) 2017-2025 Charlie Gordon
 * Xteink X4 additions: see LICENSE
 */
#ifndef XTEINK_LOADER_H
#define XTEINK_LOADER_H

#include "xteink_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Entry point: runs the launcher loop forever (never returns). */
void xteink_run_launcher(void);

#ifdef __cplusplus
}
#endif

#endif /* XTEINK_LOADER_H */
