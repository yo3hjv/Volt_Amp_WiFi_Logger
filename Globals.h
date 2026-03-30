// Shared global state used across modules.
//
// This project uses a small set of globals for:
// - Runtime telemetry (V/A/mW/mWh, log status, etc.)
// - WiFi mode and credentials
// - Calibration parameters (gain/offset)
// - User-configurable logging interval and stop conditions
//
// Persistent values are stored using Preferences and loaded at boot.

#pragma once

#include <Arduino.h>
#include <Preferences.h>

// Globals

extern Preferences g_prefs;

// System flags
extern bool g_spiffsOk;
extern bool g_inaOk;
extern bool g_loggingActive;
extern bool g_apMode;
extern bool g_staConnected;

// Timing
extern uint32_t g_bootMs;
extern uint32_t g_logStartMs;
extern uint32_t g_lastInaSampleMs;
extern uint32_t g_lastTelemetryPushMs;
extern uint32_t g_lastLogTickMs;
extern uint32_t g_lastEnergyMs;

// User settings
extern uint32_t g_userLogIntervalMs;      // Interval between CSV points and plot points
extern uint32_t g_wsRefreshMs;            // WebSocket telemetry refresh

// Stop conditions
extern bool g_stopImaxEn;
extern bool g_stopIminEn;
extern bool g_stopVmaxEn;
extern bool g_stopVminEn;
extern bool g_stopIntervalEn;
extern float g_stopImax_A;
extern float g_stopImin_A;
extern float g_stopVmax_V;
extern float g_stopVmin_V;
extern uint32_t g_stopInterval_s;
extern String g_stopReason;

// Calibration (gain)
extern float g_vGain;
extern float g_iGain;

// Calibration (offset)
extern float g_vOffset_V;
extern float g_iOffset_A;

// Measurements (raw and calibrated)
extern float g_busV_raw;
extern float g_shuntmV_raw;
extern float g_currentA_raw;
extern float g_voltageV;
extern float g_currentA;
extern float g_power_mW;
extern double g_energy_mWh;

// Averaging
extern float g_busV_avg;
extern float g_currentA_avg;

// Log pulse (for virtual blink LED)
extern volatile bool g_logWritePulse;
extern volatile uint32_t g_logPulseId;

// WiFi credentials
extern String g_staSsid;
extern String g_staPass;

// Cached WiFi scan results for /wifi page
extern String g_wifiSsidOptionsHtml;
extern volatile bool g_wifiScanRequested;

void initGlobals();
