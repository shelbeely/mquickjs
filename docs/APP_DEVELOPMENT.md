# App Development Guide — Xteink X4

This guide covers the app lifecycle, memory constraints, and best practices
for writing JavaScript apps for the Xteink X4.

---

## App Formats

| Extension | Format | Notes |
|-----------|--------|-------|
| `.js` | Raw JavaScript source | Easier to edit; parsed on-device at launch time |
| `.app` | MQuickJS 32-bit bytecode | Faster startup; smaller on-disk; preferred for distribution |

Both formats are scanned from `SDCARD/apps/` and displayed in the launcher.

### Compiling to `.app`

Use the `mqjs` host tool (built with `make mqjs`):

```bash
# Compile and enforce the 32 KB limit
./mqjs --xteink -o apps/myscript.app myscript.js

# Check size without writing a file
./mqjs --xteink --check myscript.js
```

`--xteink` automatically enables:
- 32-bit bytecode output (compatible with the ESP32-C3 RISC-V core)
- Column-info stripping (saves ~5–10% of bytecode size)
- 32 KB size enforcement with a helpful error if exceeded

---

## Memory Budget

| Resource | Limit |
|----------|-------|
| Script / bytecode file size | 32 KB |
| JS working memory (heap + stack) | 64 KB |
| Recommended file-read chunk | 4 096 bytes |

The 64 KB JS heap is a static buffer allocated once per app launch.
When a `.app` file is loaded, the ~32 KB file buffer is freed *before* the
script starts executing, so you start with close to the full 64 KB available.

---

## Keeping Memory Low

### 1. Read files in chunks

Never read a whole file into memory at once.
Use `FS.readChunk()` (default 4 KB) and process each chunk immediately:

```javascript
var fd = FS.open("/sdcard/book.txt", "r");
var chunk;
while ((chunk = FS.readChunk(fd)) !== null) {
    // Display, process, or discard chunk before the next read
    gc();  // free any temporary objects
}
FS.close(fd);
```

### 2. Call `gc()` regularly

The garbage collector runs automatically, but in long loops that create many
temporary objects (strings, arrays) it helps to call `gc()` explicitly:

```javascript
for (var i = 0; i < lines.length; i++) {
    processLine(lines[i]);
    if (i % 20 === 0) gc();
}
```

### 3. Store large assets in separate files

Instead of embedding large strings or bitmap data inside the script, save them
to separate files on the SD card and load them dynamically:

```javascript
// Bad: hardcodes 8 KB of data inside the script
var data = "AAAAAA..."; // 8 000 chars

// Good: load from a file in 4 KB chunks
var fd = FS.open("/sdcard/assets/data.bin", "r");
var chunk;
while ((chunk = FS.readChunk(fd)) !== null) { /* use chunk */ gc(); }
FS.close(fd);
```

### 4. Prefer `.app` over `.js`

Bytecode is more compact than source text for most scripts.
Compile with `mqjs --xteink` and verify the size fits with `--check`.

### 5. Split large apps

If a single script exceeds 32 KB, split it into modules and use `load()`:

```javascript
// main.js
load("/sdcard/apps/lib/utils.js");
load("/sdcard/apps/lib/ui.js");
// main logic here
```

---

## App Lifecycle

1. **Discovery** — The launcher scans `SDCARD/apps/` for `.js` and `.app` files.
2. **Selection** — The user navigates the launcher list with KEY_UP / KEY_DOWN
   and presses KEY_OK to launch.
3. **Load** — For `.app`: the file is loaded into a temporary heap buffer,
   the bytecode is relocated and registered in the JS heap, then the file
   buffer is freed.  For `.js`: the source is parsed directly into bytecode
   in the JS heap.
4. **Run** — The JS module executes.  Hardware APIs are available as global
   objects (`Display`, `Input`, `FS`, `System`, `WiFi`, `HTTP`).
5. **Exit** — When the script returns (or throws an uncaught exception), the
   launcher reclaims the JS context (`JS_FreeContext`), fully running the GC,
   and returns to the launcher menu.

---

## Error Handling

If an uncaught exception propagates out of your app, the launcher displays
the error message and stack trace on-screen and waits for a key press before
returning to the menu.

To show your own error UI, catch exceptions yourself:

```javascript
try {
    // ...
} catch (e) {
    Display.clear();
    Display.drawText(10, 100, "Error: " + e.message, 12);
    Display.refresh();
    Input.waitKey();
}
```

---

## Display Refresh Strategy

Full display refreshes (`Display.refresh()`) take ~2 seconds and flash white
then black. Use them sparingly:

- Use `Display.partialRefresh(x, y, w, h)` for small updates (clock, status).
- Batch all drawing calls before calling any refresh.
- For paginated content, refresh once per page, not once per line.

---

## WiFi and HTTP

WiFi uses significant power and memory.  Connect only when needed and
disconnect afterwards:

```javascript
if (WiFi.connect("MySSID", "password", 8000)) {
    HTTP.get("http://example.com/feed.txt", function(chunk) {
        // process chunk immediately
        gc();
    });
    WiFi.disconnect();
} else {
    Display.drawText(10, 10, "WiFi failed", 16);
    Display.refresh();
}
```

---

## Debugging

`print()` sends output to the UART (115200 baud):

```javascript
print("x =", x, "y =", y);
```

Connect with `idf.py monitor` or `screen /dev/ttyUSB0 115200`.

---

## Size Limits Quick Reference

| Item | Limit |
|------|-------|
| `.js` source file | 32 KB |
| `.app` bytecode file | 32 KB |
| JS working memory | 64 KB |
| Chunk read size | 4 KB (max recommended) |
| Max open file descriptors | 8 |
| Max apps in launcher | 64 |
