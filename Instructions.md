# LabSupplyLog – First Run / Usage Instructions

This document is a practical, step-by-step guide for the first time you flash the firmware, upload the Web UI files, and start using the device.

## 1) Prerequisites

- **ESP32 dev board** + INA219 module wired on I2C.
- **Arduino IDE** or **PlatformIO** configured for your ESP32 board.
- Libraries as used in this project:
  - ESPAsyncWebServer
  - AsyncTCP
  - ArduinoJson
  - Adafruit INA219
- A web browser (Chrome/Edge/Firefox).

## 2) Flash the firmware

1. Open the project in your IDE.
2. Select your ESP32 board and the correct COM port.
3. Build/Upload the firmware.
4. Open **Serial Monitor** at **115200 baud**.

### What you should see on Serial
- At boot you should see AP/STA messages similar to:
  - `[WiFi] AP started: ssid='Lab_Supply' ip=192.168.4.1 ...`
  - or, if STA credentials exist, it will attempt STA first.

## 3) First WiFi connection (AP mode)

On a clean first run (no saved STA credentials), the device starts in **AP mode**:

- **SSID**: `Lab_Supply`
- **Password**: none (open AP)
- **Device IP**: `192.168.4.1`

Steps:

1. On your phone/laptop, connect to the WiFi network **Lab_Supply**.
2. Open your browser:
   - `http://192.168.4.1/`

If UI files are not present in SPIFFS yet, the root page will show a message telling you to upload `index.html`, `style.css`, `app.js`.

## 4) Upload the Web UI files to SPIFFS

The Web UI files are stored in SPIFFS **root**.

### 4.1 Open the upload page

- Open:
  - `http://192.168.4.1/upload`

### 4.2 Upload files from `ui_files`

Upload these files (from the repository folder `ui_files`):

- `index.html`
- `style.css`
- `app.js`

Notes:

- Upload is **one file at a time**.
- Allowed extensions include: `html`, `css`, `js`, `png`, `ico`, `svg`, `csv`.
- Files are uploaded to SPIFFS root (no subfolders).

### 4.3 Verify

1. After uploading, open:
   - `http://192.168.4.1/`
2. You should now see the full Web UI.

If you still see the “UI files not found” message, re-check:
- the file names match exactly (`index.html`, `style.css`, `app.js`)
- the files appear in the **Web Files** list in `/upload`

## 5) (Optional) Switch to STA mode (connect to your router)

The device can run in STA mode and synchronize time via NTP.

### 5.1 Open WiFi setup page

- From the main UI, press the **WIFI** button (opens `/wifi` in a new tab), or open directly:
  - `http://192.168.4.1/wifi`

### 5.2 Scan and save credentials

1. Press **Scan**.
2. Wait ~5 seconds.
3. Reload the `/wifi` page to see results.
4. Select your SSID from the list (or enter it manually).
5. Enter your WiFi password.
6. Set action to **Save and switch to STA**.
7. Press **Apply**.

### 5.3 What happens next

- The ESP32 switches to **STA mode** and attempts to connect.
- If it fails to connect in ~10 seconds, it falls back to **AP**.

### 5.4 Finding the device in STA

Once connected to your router:

- Try mDNS:
  - `http://powersupply.local/`
- Or check your router DHCP list for the assigned IP.

## 6) Using the main Web UI

Open:

- `http://<device_ip>/`

The UI uses WebSocket for fast updates.

### 6.1 Live telemetry

You will see real-time values:

- Voltage (V)
- Current (A)
- Power (mW)
- Energy (mWh)
- Elapsed time

The header also shows:

- WebSocket connection status
- Local date/time (if NTP time is available)
- SPIFFS usage percent (warning appears when usage is high)

### 6.2 Logging controls

- **START Log**
  - starts a new CSV file in SPIFFS
  - resets cumulative energy (mWh)
- **STOP Log**
  - stops logging and closes the CSV file

### 6.3 Log interval

- `Interval (ms)` controls how often a CSV line is appended.
- The plot is updated **one point per log interval**.

### 6.4 Stop conditions

Stop conditions are optional. If enabled, logging stops automatically when a condition is reached:

- Stop I max (mA)
- Stop I min (mA)
- Stop V max (V)
- Stop V min (V)
- Stop T (s)

Leave fields empty (or disabled) if you want logging to continue until you press **STOP Log**.

## 7) Calibration (Vcal / Ical)

Calibration is done from the dedicated **Calibration** section.

### 7.1 Voltage calibration (tare + gain)

Voltage uses both an **offset (tare/zero)** and a **gain**:

- `V = Vraw * Vgain + Voffset`

Recommended procedure:

#### A) Voltage tare / zero (short to GND)

Use this to remove the small voltage reading when the input is at 0 V.

1. Make sure the INA219 **BUS (Vin+)** is at **0 V**.
   - For example, short BUS (Vin+) to GND.
2. Enter **Vcal (V) = 0**.
3. Press **CAL**.

This updates `Voffset` so that the displayed voltage becomes ~0 V under the tare conditions.

#### B) Voltage gain calibration (known voltage)

Use this to correct scaling at a real voltage.

1. Apply a stable voltage.
2. Measure it with an external meter.
3. Enter the external voltage in **Vcal (V)**.
4. Press **CAL**.

This updates `Vgain` while keeping the existing `Voffset`.

### 7.2 Current calibration (tare + gain)

Two typical actions:

- **Tare (zero current offset)**
  1. Make sure current is 0 A.
  2. Enter **Ical (A) = 0**.
  3. Press **CAL**.

- **Gain calibration (known current)**
  1. Apply a stable load.
  2. Measure current with an external meter.
  3. Enter the external current in **Ical (A)**.
  4. Press **CAL**.

Current uses:

- `I = Iraw * Igain + Ioffset`

Calibration values are saved in **Preferences** and persist across reboots.

## 8) Log files: download / delete

### 8.1 Open logs page

- From the main UI, click **LOGS**, or open:
  - `http://<device_ip>/logs`

### 8.2 Download logs

- Use the download action to fetch a CSV to your PC.

### 8.3 Delete logs

- You can delete individual logs.
- You can delete all logs (confirmation required).

### 8.4 Log naming and retention

- When NTP time is valid, logs use timestamp naming:
  - `DDMMYY_HHMMSS.csv`
- Otherwise logs fall back to:
  - `log_000x.csv`
- The firmware keeps a bounded history (about **50** logs). Older logs are pruned.

## 9) Page refresh behavior (important)

If logging is active and the Web UI disconnects (browser tab closed or refresh), the firmware stops logging automatically after a short grace period (~4 s) to avoid stopping during a quick refresh.

## 10) Troubleshooting

### UI does not load / shows “UI files not found”
- Upload `index.html`, `style.css`, `app.js` at `/upload`.
- Confirm they appear in the Web Files list.

### STA does not connect
- Verify SSID/password.
- Watch Serial Monitor for WiFi status.
- If STA fails, device falls back to AP after ~10 s.

### Scan shows no networks
- In `/wifi` press **Scan**, wait ~5 s, then reload.
- Make sure the device is in AP+STA mode when scanning (handled by firmware).

### SPIFFS is full
- Download needed logs, then delete old logs.
- Remove unused web assets from `/upload`.

---

Author: Adrian YO3HJV – March 2026
