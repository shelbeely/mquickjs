/*
 * image_viewer.js — Xteink X4 example app
 *
 * Reads a 1-bit Windows BMP file from the SD card and renders it on
 * the display using Display.drawBitmap(), row-by-row in 4 KB chunks.
 *
 * Supported input format:
 *   - Windows BMP (BITMAPFILEHEADER + BITMAPINFOHEADER)
 *   - 1 bit per pixel (pure black and white)
 *   - Width ≤ 480 px, height ≤ 800 px
 *
 * Convert any image to the required format with ImageMagick:
 *   convert input.png -resize 480x800 -monochrome BMP3:output.bmp
 *
 * Press KEY_BACK to return to the launcher.
 */

var SDCARD  = "/sdcard";
var MARGIN  = 12;
var FONT_SM = 12;
var FONT_MD = 16;

/* ---- Utility: read exactly n bytes from fd into a Uint8Array ---- */
function readExact(fd, n) {
    var buf = new Uint8Array(n);
    var got = FS.read(fd, buf, 0, n);
    if (got !== n) return null;
    return buf;
}

/* ---- Read a little-endian 32-bit integer from a Uint8Array ---- */
function readU32LE(buf, off) {
    return ((buf[off + 3] << 24) | (buf[off + 2] << 16) |
            (buf[off + 1] <<  8) |  buf[off]) >>> 0;
}

function readU16LE(buf, off) {
    return (buf[off + 1] << 8) | buf[off];
}

/* ---- Pick a .bmp file from the SD card ---- */
function pickBmp() {
    var files = FS.list(SDCARD);
    var bmps = [];
    for (var i = 0; i < files.length; i++) {
        var f = files[i];
        if (f.length > 4 &&
            (f.slice(f.length - 4).toLowerCase() === ".bmp")) {
            bmps.push(f);
        }
    }

    if (bmps.length === 0) {
        Display.clear();
        Display.setColor(0);
        Display.drawText(MARGIN, 20, "No .bmp files found in " + SDCARD, FONT_MD);
        Display.drawText(MARGIN, 50, "Convert an image with ImageMagick:", FONT_SM);
        Display.drawText(MARGIN, 68,
            "convert in.png -resize 480x800 -monochrome BMP3:out.bmp",
            FONT_SM);
        Display.refresh();
        Input.waitKey();
        return null;
    }

    var sel = 0;
    var LINE_H = FONT_SM + 4;
    var needsDraw = true;

    while (true) {
        if (needsDraw) {
            Display.clear();
            Display.setColor(0);
            Display.fillRect(0, 0, Display.width, 28);
            Display.setColor(3);
            Display.drawText(MARGIN, 6, "Select a BMP", FONT_MD);
            Display.setColor(0);

            var y = 40;
            for (var i = 0; i < bmps.length; i++) {
                if (i === sel) {
                    Display.fillRect(MARGIN - 4, y - 2,
                                     Display.width - MARGIN, LINE_H + 2);
                    Display.setColor(3);
                } else {
                    Display.setColor(0);
                }
                Display.drawText(MARGIN, y, bmps[i], FONT_SM);
                y += LINE_H + 2;
            }
            Display.refresh();
            needsDraw = false;
        }

        var key = Input.waitKey();
        if (key === Input.KEY_UP && sel > 0)              { sel--; needsDraw = true; }
        else if (key === Input.KEY_DOWN && sel < bmps.length - 1) { sel++; needsDraw = true; }
        else if (key === Input.KEY_OK)   { return SDCARD + "/" + bmps[sel]; }
        else if (key === Input.KEY_BACK) { return null; }
    }
}

/* ---- Render the BMP ---- */
function showBmp(path) {
    var fd = FS.open(path, "r");
    if (fd < 0) {
        Display.clear();
        Display.drawText(MARGIN, 20, "Cannot open: " + path, FONT_MD);
        Display.refresh();
        Input.waitKey();
        return;
    }

    /* Read BMP file header (14 bytes) */
    var fhdr = readExact(fd, 14);
    if (!fhdr || fhdr[0] !== 0x42 || fhdr[1] !== 0x4D) {  /* 'BM' */
        FS.close(fd);
        Display.clear();
        Display.drawText(MARGIN, 20, "Not a valid BMP file.", FONT_MD);
        Display.refresh();
        Input.waitKey();
        return;
    }
    var pixelOffset = readU32LE(fhdr, 10);

    /* Read DIB header (40 bytes, BITMAPINFOHEADER) */
    var ihdr = readExact(fd, 40);
    if (!ihdr) { FS.close(fd); return; }
    var imgW  = readU32LE(ihdr, 4);
    var imgH  = readU32LE(ihdr, 8);  /* positive = bottom-up */
    var bpp   = readU16LE(ihdr, 14);

    if (bpp !== 1) {
        FS.close(fd);
        Display.clear();
        Display.drawText(MARGIN, 20, "Only 1-bit BMP supported.", FONT_MD);
        Display.drawText(MARGIN, 45,
            "Use: convert in.png -monochrome BMP3:out.bmp", FONT_SM);
        Display.refresh();
        Input.waitKey();
        return;
    }

    /* BMP rows are padded to a multiple of 4 bytes */
    var rowStride = (((imgW + 31) >> 5) << 2);  /* bytes per row (4-byte aligned) */

    /* Centre the image on the display */
    var dx = ((Display.width  - imgW) / 2) | 0;
    var dy = ((Display.height - imgH) / 2) | 0;

    Display.clear();

    /* Seek to pixel data */
    FS.seek(fd, pixelOffset, 0);

    /* BMP stores rows bottom-up; collect into an array of row buffers
     * and render from bottom to top.  We process one row at a time to
     * keep memory usage low. */
    var rowBuf = new Uint8Array(rowStride);

    for (var row = 0; row < imgH; row++) {
        var got = FS.read(fd, rowBuf, 0, rowStride);
        if (got < rowStride) break;

        /* BMP is bottom-up → display row (imgH-1-row) */
        var displayRow = dy + (imgH - 1 - row);
        if (displayRow >= 0 && displayRow < Display.height) {
            Display.drawBitmap(dx, displayRow, rowBuf, imgW, 1);
        }

        if (row % 50 === 0) gc();
    }
    FS.close(fd);

    Display.refresh();

    /* Wait for key press */
    Display.setColor(2);
    Display.drawText(MARGIN, Display.height - 18, "Press any key", FONT_SM);
    Display.partialRefresh(0, Display.height - 24, Display.width, 24);
    Input.waitKey();
}

/* ---- Main ---- */
var path = pickBmp();
if (path !== null) {
    showBmp(path);
}
