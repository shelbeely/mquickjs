/*
 * hello_world.js — Xteink X4 example app
 *
 * Clears the display, draws "Hello, World!" centred on the panel,
 * triggers a full refresh, then waits for any key press before exiting.
 *
 * Place this file in SDCARD/apps/ and select it from the launcher.
 */

var TEXT   = "Hello, World!";
var FONT   = 24;             // font size (pixels)
var TEXT_W = TEXT.length * (FONT / 2);  // approximate text width

var cx = (Display.width  - TEXT_W) / 2 | 0;
var cy = (Display.height - FONT)   / 2 | 0;

Display.clear();
Display.setColor(0);  // black
Display.drawText(cx, cy, TEXT, FONT);
Display.refresh();

// Show a small hint at the bottom
Display.setColor(2);  // light gray
Display.drawText(10, Display.height - 30, "Press any key to exit", 12);
Display.partialRefresh(0, Display.height - 40, Display.width, 40);

Input.waitKey();
