# Getting Started with MQuickJS for Xteink X4

This guide walks you through setting up the toolchain, building the firmware,
flashing your device, and preparing a microSD card with your first app.

---

## Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| GCC  | ≥ 11    | For building the host-side `mqjs` compiler |
| ESP-IDF | 5.x  | [Installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/get-started/) |
| GNU Make | any | Comes with most Linux/macOS setups |
| Python 3 | ≥ 3.8 | Required by ESP-IDF |

---

## 1. Clone the Repository

```bash
git clone https://github.com/shelbeely/mquickjs.git
cd mquickjs
```

---

## 2. Build the Host-side Compiler

The `mqjs` tool runs on your development machine and compiles `.js` files to
`.app` bytecode for the Xteink X4.

```bash
make mqjs
```

Verify it works:

```bash
./mqjs --help
./mqjs --xteink --check examples/hello_world.js
```

---

## 3. Build the Xteink Stdlib Header

Before building the firmware you must generate the ROM stdlib table that
embeds the hardware API into the JS runtime:

```bash
make xteink_stdlib.h
```

This produces `xteink_stdlib.h` which is included by the firmware.

---

## 4. Configure the Firmware

Open `xteink/xteink_config.h` and verify the GPIO pin assignments match your
specific PCB revision (compare with the schematic at
<https://github.com/sunwoods/Xteink-X4>):

```c
#define XTEINK_GPIO_KEY_UP   4
#define XTEINK_GPIO_KEY_DOWN 5
// ...
```

---

## 5. Build the Firmware

### Using ESP-IDF (recommended)

```bash
cd xteink
idf.py set-target esp32c3
idf.py build
```

### Using PlatformIO

```bash
# Generate xteink_stdlib.h first (from repo root)
make xteink_stdlib.h

# Then build with PlatformIO
pio run
```

---

## 6. Flash the Device

Connect your Xteink X4 via USB-C.  Hold the **BOOT** button (GPIO0), press
**RESET**, then release BOOT to enter download mode.

```bash
# ESP-IDF
idf.py -p /dev/ttyUSB0 flash monitor

# PlatformIO
pio run --target upload --upload-port /dev/ttyUSB0
```

> **Alternative**: You can also use the browser-based WebUSB flasher from the
> CrossPoint Reader project (<https://crosspointreader.com/>).

---

## 7. Prepare the SD Card

1. Format a microSD card as **FAT32**.
2. Create the directory `apps/` at the root of the card.
3. Copy your `.js` or `.app` files into `apps/`.

```
SD card:
└── apps/
    ├── hello_world.js
    ├── clock.js
    └── file_viewer.app
```

4. Insert the card and power on the device.  The launcher will appear.

---

## 8. Write Your First App

Create `hello.js`:

```javascript
Display.clear();
Display.setColor(0);  // black
Display.drawText(40, 380, "Hello, Xteink!", 16);
Display.refresh();

// Wait for any button press, then exit
Input.waitKey();
```

Copy it to the SD card's `apps/` directory, or compile it first:

```bash
./mqjs --xteink -o hello.app hello.js
```

Then copy `hello.app` to `apps/` on the SD card.

---

## 9. Serial Debugging

`print()` and `console.log()` output is forwarded to the UART (115200 baud).
Connect with any serial terminal:

```bash
idf.py monitor
# or
screen /dev/ttyUSB0 115200
```

---

## See Also

- [JavaScript API Reference](JAVASCRIPT_API.md)
- [App Development Guide](APP_DEVELOPMENT.md)
- [Example App Walkthroughs](EXAMPLES.md)
