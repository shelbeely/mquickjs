/*
 * button_demo.js — Xteink X4 example app
 *
 * Displays the name of the last button pressed.
 * Uses Display.partialRefresh() so only the changed text area is
 * redrawn, avoiding the slow full-panel flash.
 *
 * Press KEY_BACK to return to the launcher.
 */

var KEY_NAMES = ["UP", "DOWN", "BACK", "OK", "POWER"];

var LABEL_Y  = 360;
var FONT     = 24;
var MARGIN   = 20;
var AREA_H   = FONT + 20;

/* Draw the static title once */
Display.clear();
Display.setColor(0);
Display.drawText(MARGIN, 20, "Button Demo", FONT);
Display.drawText(MARGIN, 60, "Press any button.", 16);
Display.drawText(MARGIN, 90, "Press BACK to exit.", 16);
Display.refresh();

/* Event loop */
var running = true;
while (running) {
    var key = Input.waitKey();

    /* Erase the previous key name and draw the new one */
    Display.setColor(3);  // white — erase
    Display.fillRect(MARGIN, LABEL_Y, Display.width - MARGIN * 2, AREA_H);

    Display.setColor(0);  // black — draw
    var name = KEY_NAMES[key] || "UNKNOWN";
    Display.drawText(MARGIN, LABEL_Y + 4, "Last key: " + name, FONT);
    Display.partialRefresh(MARGIN, LABEL_Y, Display.width - MARGIN * 2, AREA_H);

    if (key === Input.KEY_BACK) {
        running = false;
    }
}
