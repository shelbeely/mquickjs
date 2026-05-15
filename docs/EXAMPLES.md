# Example Apps — Walkthrough

All example apps are in the `examples/` directory.
Copy them to `SDCARD/apps/` on your microSD card, or compile to `.app` first:

```bash
./mqjs --xteink -o apps/hello_world.app examples/hello_world.js
```

---

## `hello_world.js`

**What it does**: Clears the screen, draws "Hello, World!" centred on the
display, triggers a full refresh, then waits for a key press.

**Key APIs**: `Display.clear`, `Display.drawText`, `Display.refresh`,
`Input.waitKey`

**Concepts shown**: Basic display workflow; always end with `refresh()`.

---

## `button_demo.js`

**What it does**: Draws the name of the last button pressed and loops until
KEY_BACK is pressed.

**Key APIs**: `Input.waitKey`, `Input.KEY_*` constants, `Display.partialRefresh`

**Concepts shown**:
- `Input.waitKey()` blocks until any button event.
- `Display.partialRefresh()` redraws only the changed text area, avoiding the
  slow full-panel flash.

---

## `clock.js`

**What it does**: Shows a running clock that updates its digits every second.

**Key APIs**: `System.millis`, `System.sleep`, `Display.partialRefresh`

**Concepts shown**:
- `System.millis()` for elapsed time.
- Partial refresh for the digit area only — much faster than a full refresh.
- Infinite loop with `System.sleep(1000)` to yield CPU.

---

## `file_viewer.js`

**What it does**:
1. Lists `.txt` files in `/sdcard/`.
2. Lets the user select one with the page-turn buttons.
3. Displays the content page-by-page, reading 4 KB at a time.

**Key APIs**: `FS.list`, `FS.open`, `FS.readChunk`, `FS.close`, `Input.waitKey`,
`Display.refresh`, `gc()`

**Concepts shown**:
- `FS.list()` to build a file picker.
- `FS.readChunk()` to stream a file without loading it all at once.
- `gc()` called between pages to release the previous page's string data.
- Graceful handling of empty directories.

---

## `image_viewer.js`

**What it does**: Reads a 1-bit BMP file from the SD card and renders it on
the display row-by-row using `Display.drawBitmap`.

**Key APIs**: `FS.open`, `FS.read`, `FS.seek`, `FS.close`, `Display.drawBitmap`,
`Display.refresh`

**Concepts shown**:
- Reading a binary file header with `FS.read()`.
- Seeking to the pixel data offset from the BMP header.
- Processing image data in 4 KB chunks (one or more rows at a time) to stay
  within the 64 KB memory budget.
- Converting the BMP 4-byte row alignment back to tight packing for
  `Display.drawBitmap`.

---

## Tips for Writing Your Own Apps

1. **Start with `Display.clear()`** to ensure a blank slate.
2. **End with `Display.refresh()`** or `Display.partialRefresh()` — nothing
   appears on screen until you refresh.
3. **Wrap in try/catch** if you do file or network I/O so the user sees a
   friendly message instead of the raw stack trace.
4. **Always close file descriptors** with `FS.close(fd)` when done.
5. **Compile to `.app`** with `mqjs --xteink -o myapp.app myapp.js` for
   faster startup and a smaller on-disk footprint.
