# INTERNAL DEBUG NOTES (LabSupplyLog_V1)

This file is intended as an internal reference for debugging and maintenance.

## Project scope

- ESP32-based lab supply logger using INA219 for voltage/current measurements.
- Web UI served from SPIFFS root (`index.html`, `style.css`, `app.js`).
- Async web server + WebSocket for telemetry/control.
- CSV logging to SPIFFS with stop conditions and log management pages.

## Entry points

- `LabSupplyLog_V1.ino`
  - `setup()` initializes:
    - globals (`initGlobals()`)
    - SPIFFS (`initStorage()`)
    - WiFi (`initWiFi()`)
    - INA219 sampling (`initIna()`)
    - Web server (`initWebServer()`)
    - WebSocket (`initWebSocket()`)
    - then starts server (`startWebServer()`)
  - `loop()` calls:
    - `tickWiFi()`
    - `tickIna()`
    - `tickLogging()`
    - `tickTelemetry()`

## Core modules

- `Globals.h/.cpp`
  - Owns runtime state + persisted preferences.
  - Key user parameters:
    - `g_userLogIntervalMs` (default 1000 ms)
    - `g_wsRefreshMs` (default 200 ms)
  - Calibration parameters:
    - `g_vGain`, `g_iGain`
    - `g_vOffset_V`, `g_iOffset_A`

- `InaMgr.*`
  - Periodic INA219 sampling + averaging.
  - Applies calibration to produce displayed values:
    - `V = Vraw * Vgain + Voffset`
    - `I = Iraw * Igain + Ioffset`

- `WsMgr.*`
  - WebSocket endpoint `/ws`.
  - Telemetry messages include: `V`, `A`, `mW`, `mWh`, `elapsed_s`, `date`, `time`, `spiffs_used_pct`, `logging`, `pulse_id`.
  - Handles commands from UI:
    - `start_log`, `stop_log`
    - `cal` (supports voltage and current calibration)

- `LogMgr.*`
  - Creates a CSV file in SPIFFS when logging starts.
  - CSV header: `elapsed_s,V,A,mW,mWh`.
  - Naming:
    - Prefer timestamp: `/DDMMYY_HHMMSS.csv` (local time if NTP valid)
    - Fallback: `/log_000x.csv`
  - Retention: prunes to ~50 log files.
  - Stop conditions:
    - Imax/Imin/Vmax/Vmin/time.
  - IMPORTANT (flash wear/perf):
    - writes 1 line per interval (`g_userLogIntervalMs`)
    - flush is periodic every ~3000 ms (`kLogFlushIntervalMs`)
    - flush+close on stop.

- `UploadMgr.*` / `UploadPage.*`
  - `/upload` page for uploading UI assets and managing files.

- `WifiMgr.*` / `WifiWeb.*` / `WifiPage.*`
  - `/wifi` page for scan/save credentials and AP/STA switching.
  - AP is open (no password).

- `WebMgr.*`
  - Registers routes and serves SPIFFS files.

## Web routes (high level)

- `/` main UI (from SPIFFS)
- `/ws` WebSocket
- `/wifi` WiFi configuration
- `/upload` file upload/management
- `/logs` logs management page (if enabled by UploadMgr/LogMgr)
- `/api/logs` list log files
- `/api/logs/delete_all` delete all logs (POST)
- `/log?name=log_0001.csv` download log

## Web UI files

- `ui_files/index.html`
  - Interval input `inpInterval`.
  - Note next to label: `Recomm.: >=200ms.`

- `ui_files/app.js`
  - Displays telemetry and handles WS messages.
  - Autoscale steps:
    - Voltage: `[3, 6, 10, 15, 20, 26, 35]`
    - Current: `[0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 1.5, 3.0]`
  - Grid styling (dotted markers) is darker (higher alpha).
  - Display formatting:
    - `mWh` shown with 2 decimals.

## Calibration behavior (important)

- Voltage calibration via WS `cal` command:
  - If `Vcal ~= 0` => tare/zero:
    - `Voffset = -rawV * Vgain` so displayed voltage becomes ~0 at that moment.
  - If `Vcal > 0` and rawV above small threshold => gain:
    - `Vgain = (Vcal - Voffset) / rawV` (keeps existing offset).

- Current calibration:
  - Supports tare (`Ical=0`) and gain similarly.

## Logging + UI disconnect behavior

- Logging stops automatically after a grace period when no WS clients remain (implemented in WS manager).
- This prevents leaving log files open on browser refresh/close.

## Physical log tick LED (optional feature added)

- GPIO13 active-low pulse ~20 ms on each CSV line write.
- Pin is initialized in `LabSupplyLog_V1.ino`.
- Pulse is generated in `LogMgr.cpp` and turned off non-blocking using `millis()`.
- Safety: LED forced OFF in `startLogging()` and `stopLogging()` to avoid sticking ON.

## Common debug checkpoints

- UI not loading:
  - verify `index.html`, `style.css`, `app.js` are in SPIFFS root and accessible from `/upload`.

- Log interval too low:
  - UI recommends >=200 ms.
  - If set too low, CPU/flash load increases and UI may appear less responsive.

- WiFi issues:
  - AP SSID should be `Lab_Supply`.
  - AP is open.
  - `/wifi` scan results require waiting and reloading.

- Autoscale jumps:
  - Threshold selection uses `x <= step` => a value slightly over a step will go to the next step.

---

Author: Adrian YO3HJV – March 2026
