# LabSupplyLog (ESP32 + INA219)

ESP32-based lab power supply logger.

The firmware measures voltage/current using an INA219 module and exposes a Web UI over WiFi for live telemetry, calibration, and CSV logging to SPIFFS.

## Hardware

- **MCU**: ESP32 dev board
- **Sensor**: INA219 current/voltage monitor (I2C)
- **Current sense resistor (shunt)**: 0.33 Ω (as used in this project)

### Wiring (typical ESP32 defaults)

INA219 connects via I2C:

- **SDA**: GPIO 21 (default `Wire` SDA on many ESP32 boards)
- **SCL**: GPIO 22 (default `Wire` SCL on many ESP32 boards)
- **VCC**: 3.3 V
- **GND**: GND
- **GPIO13**: Write LED. To +3V3 via limiting resistor; pulse when ta is written on SPIFFS.

If your ESP32 board uses different default I2C pins, adjust accordingly.

## What it does

- **INA219 sampling** (~50 ms) with IIR averaging for smoother UI readings.
- **Web UI** (served from SPIFFS) with:
  - live Voltage/Current/Power/Energy/Elapsed
  - plot (one point per log interval)
  - calibration inputs and CAL action
  - links to WiFi page and log management
- **WebSocket telemetry** for fast UI updates.
- **CSV logging to SPIFFS**:
  - Columns: `elapsed_s,V,A,mW,mWh`
  - Filename:
    - preferred: timestamp based `DDMMYY_HHMMSS.csv` when local time is valid (NTP)
    - fallback: `log_000x.csv`
  - Retention: keep up to ~50 log files (older files are pruned)
- **Stop conditions** (configured from UI):
  - Current max/min, Voltage max/min, Time max
- **Calibration**:
  - Voltage: gain-only
    - `V = Vraw * Vgain`
  - Current: gain + offset (tare)
    - `I = Iraw * Igain + Ioffset`
    - `Ical = 0` triggers tare (offset)
- **WiFi AP/STA management**:
  - AP SSID: `Lab_Supply` (open, no password)
  - `/wifi` page for scanning networks and saving STA credentials
  - On boot: if saved STA credentials exist, try STA; fallback to AP after ~10 s if not connected
- **NTP time sync** in STA mode with Romania local time/DST.

## Web pages / endpoints

- `/` main UI (from SPIFFS)
- `/wifi` WiFi setup and scan
- `/logs` log management (download/delete)
- `/upload` asset upload / SPIFFS web files management
- WebSocket: `/ws`

## Build environment

Verified setup (per project notes):

- ESPAsyncWebServer v3.7.10
- AsyncTCP v3.4.5
- Arduino ESP32 Boards: 2.0.18
- ESP32 by Espressif: 2.0.9

## Notes

- The firmware uses **SPIFFS** (root). Web assets (HTML/CSS/JS) and log CSV files are stored in the SPIFFS root.
- Pages include an author footer: `Author: Adrian YO3HJV - March 2026`.
