# JavaScript API Reference — Xteink X4

All APIs are available as global objects in every app.
Apps run as ES5 JavaScript in MQuickJS with a **64 KB working memory limit**.

---

## Global Functions

| Function | Description |
|----------|-------------|
| `print(...args)` | Print arguments to the UART (serial debug output) |
| `console.log(...args)` | Alias for `print()` |
| `gc()` | Trigger garbage collection; call in long loops to keep memory low |
| `load(path)` | Load and execute another `.js` file from the SD card |

---

## `Display`

The 4.3″ e-ink display (480 × 800 px, 4-gray).

### Constants

| Property | Value | Description |
|----------|-------|-------------|
| `Display.width` | `480` | Panel width in pixels |
| `Display.height` | `800` | Panel height in pixels |

### Color constants (use with `Display.setColor`)

| Value | Color |
|-------|-------|
| `0` | Black |
| `1` | Dark gray |
| `2` | Light gray |
| `3` | White |

### Methods

#### `Display.clear()`
Fill the display with white and reset the draw color to black.

#### `Display.setColor(c: number)`
Set the current draw color (0–3).

#### `Display.drawText(x, y, text, fontSize)`
Draw a UTF-8 string at pixel position `(x, y)` using the given font size.
Supported sizes: `12`, `16`, `24`.

#### `Display.drawRect(x, y, w, h)`
Draw an unfilled rectangle outline.

#### `Display.fillRect(x, y, w, h)`
Draw a filled rectangle.

#### `Display.drawLine(x1, y1, x2, y2)`
Draw a straight line.

#### `Display.drawBitmap(x, y, data, w, h)`
Render a 1-bit-per-pixel bitmap.
`data` must be a `Uint8Array` of length `⌈w/8⌉ × h`.
Bit `1` → draw with current color; bit `0` → transparent.

#### `Display.refresh()`
Trigger a **full** e-ink panel refresh. Slow (~2 s) but produces the cleanest
image; use sparingly.

#### `Display.partialRefresh(x, y, w, h)`
Trigger a **partial** refresh of the given region. Faster than a full refresh;
suited for clock digits, status bars, etc.

---

## `Input`

Physical buttons on the device.

### Key constants

| Constant | Value | Button |
|----------|-------|--------|
| `Input.KEY_UP` | `0` | Page-up button |
| `Input.KEY_DOWN` | `1` | Page-down button |
| `Input.KEY_BACK` | `2` | Back button |
| `Input.KEY_OK` | `3` | OK / Select button |
| `Input.KEY_POWER` | `4` | Power button |

### Methods

#### `Input.waitKey() → number`
Block until a button is pressed; return the `Input.KEY_*` constant.

#### `Input.isPressed(key) → boolean`
Non-blocking: return `true` if the given key is currently held.

---

## `FS`

File system access on the microSD card.
All paths must be absolute and start with `/sdcard/`.

> **Memory tip**: Always read files in chunks of ≤ 4096 bytes.
> Never load an entire file into memory at once.

### Methods

#### `FS.open(path, mode) → fd`
Open a file and return a file descriptor (positive integer on success, `-1` on
failure).

| Mode | Meaning |
|------|---------|
| `"r"` | Read-only; file must exist |
| `"w"` | Write; truncate or create |
| `"a"` | Append; create if absent |

#### `FS.close(fd)`
Close the file descriptor.

#### `FS.read(fd, buffer, offset, length) → bytesRead`
Read `length` bytes from `fd` into `buffer` (a `Uint8Array`) starting at
`buffer[offset]`.

#### `FS.readChunk(fd, size?) → Uint8Array | null`
Read up to `size` bytes (default: `4096`). Returns `null` at end-of-file.
This is the **recommended** way to read files.

```javascript
var fd = FS.open("/sdcard/data.txt", "r");
var chunk;
while ((chunk = FS.readChunk(fd)) !== null) {
    // process chunk (Uint8Array)
    gc();  // keep memory pressure low
}
FS.close(fd);
```

#### `FS.write(fd, buffer, offset, length) → bytesWritten`
Write `length` bytes from `buffer[offset]` to `fd`.

#### `FS.seek(fd, offset, whence) → newPosition`
Seek within a file.

| `whence` | Meaning |
|----------|---------|
| `0` | From beginning |
| `1` | From current position |
| `2` | From end |

#### `FS.size(path) → number`
Return file size in bytes, or `-1` if the file does not exist.

#### `FS.exists(path) → boolean`
Return `true` if the path exists.

#### `FS.list(path) → string[]`
Return an array of filenames (not full paths) inside `path`.

#### `FS.remove(path)`
Delete a file.

#### `FS.mkdir(path)`
Create a directory.

---

## `System`

Device system information and control.

### Properties

| Property | Type | Description |
|----------|------|-------------|
| `System.version` | `string` | Firmware version (e.g. `"v1.0.0"`) |

### Methods

#### `System.millis() → number`
Milliseconds since boot (wraps after ~49 days).

#### `System.sleep(ms)`
Pause execution for `ms` milliseconds.

#### `System.battery() → number`
Battery percentage (0–100).

#### `System.reboot()`
Perform a soft reboot.

---

## `WiFi`

802.11b/g/n 2.4 GHz wireless networking.

### Methods

#### `WiFi.connect(ssid, password, timeoutMs?) → boolean`
Connect to a WiFi network.  `timeoutMs` defaults to 10 000.
Returns `true` on success.

#### `WiFi.disconnect()`
Disconnect from the current network.

#### `WiFi.isConnected() → boolean`
Return `true` if currently connected.

#### `WiFi.ipAddress() → string`
Return the current IP address as a string (e.g. `"192.168.1.42"`).

---

## `HTTP`

HTTP client with chunked delivery to keep memory usage bounded.

> **Memory tip**: The `chunkCallback` is called with a small `Uint8Array` for
> each received chunk (≤ 4096 bytes).  Never accumulate all chunks into a
> single array; process and discard each one.

### Methods

#### `HTTP.get(url, chunkCallback) → totalBytes`
Perform an HTTP GET.  `chunkCallback(data: Uint8Array)` is called for each
chunk.  Returns the total number of bytes received.

```javascript
if (WiFi.connect("MySSID", "password")) {
    HTTP.get("http://example.com/data.txt", function(chunk) {
        // process chunk
        gc();
    });
}
```

#### `HTTP.post(url, body, chunkCallback) → totalBytes`
Perform an HTTP POST with the given string `body`.

---

## Error Handling

If a JS exception propagates out of the top level of an app, the launcher
catches it, displays the error message and stack trace on screen, and
returns to the app selection menu.

To handle errors within an app, use standard `try`/`catch`:

```javascript
try {
    var fd = FS.open("/sdcard/missing.txt", "r");
    // ...
} catch (e) {
    Display.drawText(10, 10, "Error: " + e.message, 12);
    Display.refresh();
    Input.waitKey();
}
```
