/*
 * clock.js — Xteink X4 example app
 *
 * Displays a running digital clock (MM:SS since boot) that updates
 * its digits every second using Display.partialRefresh() to avoid
 * the slow full-panel flash.
 *
 * Press KEY_BACK to return to the launcher.
 */

var FONT   = 24;
var MARGIN = 20;

/* Draw static UI once */
Display.clear();
Display.setColor(0);
Display.drawText(MARGIN, 20, "Clock (time since boot)", 16);
Display.drawText(MARGIN, Display.height - 30, "BACK to exit", 12);
Display.refresh();

/* Clock area */
var CLOCK_Y = (Display.height / 2 - FONT / 2) | 0;
var CLOCK_X = (Display.width  / 2 - FONT * 3) | 0;
var CLOCK_W = FONT * 6;
var CLOCK_H = FONT + 10;

function pad2(n) {
    return (n < 10 ? "0" : "") + n;
}

var running = true;

/* Non-blocking key check is not supported in this simple example;
 * we use a minimal sleep + waitKey pattern by checking isPressed. */
while (running) {
    var ms    = System.millis();
    var secs  = (ms / 1000) | 0;
    var mins  = (secs / 60) | 0;
    var s     = secs % 60;
    var m     = mins % 60;

    /* Erase and redraw the clock digits */
    Display.setColor(3);  // white
    Display.fillRect(CLOCK_X - 4, CLOCK_Y - 4, CLOCK_W + 8, CLOCK_H + 8);
    Display.setColor(0);  // black
    Display.drawText(CLOCK_X, CLOCK_Y, pad2(m) + ":" + pad2(s), FONT);
    Display.partialRefresh(CLOCK_X - 4, CLOCK_Y - 4, CLOCK_W + 8, CLOCK_H + 8);

    gc();

    /* Wait ~1 s, but bail out immediately if BACK is pressed */
    var deadline = System.millis() + 1000;
    while (System.millis() < deadline) {
        if (Input.isPressed(Input.KEY_BACK)) {
            running = false;
            break;
        }
        System.sleep(50);
    }
}
