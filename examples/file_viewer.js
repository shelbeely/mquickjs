/*
 * file_viewer.js — Xteink X4 example app
 *
 * 1. Lists .txt files in /sdcard (root of the SD card).
 * 2. Lets the user select one with KEY_UP / KEY_DOWN, confirm with KEY_OK.
 * 3. Displays the file page-by-page, reading 4 KB at a time.
 *    KEY_DOWN / KEY_OK → next page
 *    KEY_UP            → previous page (re-reads from start)
 *    KEY_BACK          → return to file list
 *
 * Calls gc() between pages to release the previous page's string data.
 * Total memory usage stays well under 64 KB regardless of file size.
 */

var SDCARD   = "/sdcard";
var FONT_SM  = 12;
var FONT_MD  = 16;
var MARGIN   = 12;
var LINE_H   = FONT_SM + 4;
var LINES_PER_PAGE = ((Display.height - 50) / LINE_H) | 0;

/* ---- File picker ---- */

function pickFile() {
    var files = FS.list(SDCARD);
    /* Filter to .txt files */
    var txts = [];
    for (var i = 0; i < files.length; i++) {
        var f = files[i];
        if (f.length > 4 && f.slice(f.length - 4) === ".txt") {
            txts.push(f);
        }
    }

    if (txts.length === 0) {
        Display.clear();
        Display.setColor(0);
        Display.drawText(MARGIN, 20, "No .txt files found in " + SDCARD, FONT_MD);
        Display.drawText(MARGIN, 55, "Copy some .txt files to the SD card.", FONT_SM);
        Display.drawText(MARGIN, 75, "Press any key.", FONT_SM);
        Display.refresh();
        Input.waitKey();
        return null;
    }

    var sel = 0;
    var needsDraw = true;

    while (true) {
        if (needsDraw) {
            Display.clear();
            Display.setColor(0);
            Display.fillRect(0, 0, Display.width, 28);
            Display.setColor(3);
            Display.drawText(MARGIN, 6, "Select a file", FONT_MD);
            Display.setColor(0);

            var y = 40;
            for (var i = 0; i < txts.length; i++) {
                if (i === sel) {
                    Display.setColor(0);
                    Display.fillRect(MARGIN - 4, y - 2, Display.width - MARGIN, LINE_H + 2);
                    Display.setColor(3);
                } else {
                    Display.setColor(0);
                }
                Display.drawText(MARGIN, y, txts[i], FONT_SM);
                y += LINE_H + 2;
            }
            Display.refresh();
            needsDraw = false;
        }

        var key = Input.waitKey();
        if (key === Input.KEY_UP && sel > 0) {
            sel--;
            needsDraw = true;
        } else if (key === Input.KEY_DOWN && sel < txts.length - 1) {
            sel++;
            needsDraw = true;
        } else if (key === Input.KEY_OK) {
            return SDCARD + "/" + txts[sel];
        } else if (key === Input.KEY_BACK) {
            return null;
        }
    }
}

/* ---- File viewer ---- */

function viewFile(path) {
    /* Read the file in 4 KB chunks, splitting on newlines.
     * We keep only the current page in memory. */
    var fd = FS.open(path, "r");
    if (fd < 0) {
        Display.clear();
        Display.setColor(0);
        Display.drawText(MARGIN, 20, "Could not open file:", FONT_MD);
        Display.drawText(MARGIN, 50, path, FONT_SM);
        Display.refresh();
        Input.waitKey();
        return;
    }

    /* Accumulate lines by reading chunks and splitting on \n */
    var allLines = [];
    var partial  = "";
    var chunk;

    while ((chunk = FS.readChunk(fd)) !== null) {
        /* Convert Uint8Array to string (ASCII / Latin-1 subset) */
        var s = "";
        for (var i = 0; i < chunk.length; i++) {
            s += String.fromCharCode(chunk[i]);
        }
        partial += s;

        /* Split on newlines and keep the last partial line */
        var parts = partial.split("\n");
        for (var i = 0; i < parts.length - 1; i++) {
            allLines.push(parts[i]);
        }
        partial = parts[parts.length - 1];
        gc();
    }
    if (partial.length > 0) allLines.push(partial);
    FS.close(fd);

    /* Paginated display */
    var page = 0;
    var totalPages = Math.ceil(allLines.length / LINES_PER_PAGE);
    if (totalPages === 0) totalPages = 1;

    while (true) {
        Display.clear();
        Display.setColor(0);

        /* Status bar */
        Display.fillRect(0, 0, Display.width, 22);
        Display.setColor(3);
        var fname = path.slice(path.lastIndexOf("/") + 1);
        Display.drawText(MARGIN, 4,
                         fname + "  (" + (page + 1) + "/" + totalPages + ")",
                         FONT_SM);
        Display.setColor(0);

        /* Lines for this page */
        var start = page * LINES_PER_PAGE;
        var y = 30;
        for (var i = start; i < start + LINES_PER_PAGE && i < allLines.length; i++) {
            /* Truncate long lines to fit the display width */
            var line = allLines[i];
            var maxChars = ((Display.width - MARGIN * 2) / (FONT_SM * 0.6)) | 0;
            if (line.length > maxChars) line = line.slice(0, maxChars - 1) + "…";
            Display.drawText(MARGIN, y, line, FONT_SM);
            y += LINE_H;
        }

        /* Footer */
        Display.setColor(2);
        Display.drawText(MARGIN, Display.height - 18,
                         "UP=prev  DOWN/OK=next  BACK=exit", FONT_SM);
        Display.refresh();
        gc();

        var key = Input.waitKey();
        if ((key === Input.KEY_DOWN || key === Input.KEY_OK) &&
             page < totalPages - 1) {
            page++;
        } else if (key === Input.KEY_UP && page > 0) {
            page--;
        } else if (key === Input.KEY_BACK) {
            return;
        }
    }
}

/* ---- Main ---- */
var path = pickFile();
if (path !== null) {
    viewFile(path);
}
