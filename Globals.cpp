// Definitions for the shared globals declared in Globals.h.
//
// This file owns:
// - Default values for runtime state
// - Preferences initialization and loading persisted settings at boot

#include "Globals.h"

Preferences g_prefs;

bool g_spiffsOk = false;
bool g_inaOk = false;
bool g_loggingActive = false;
bool g_apMode = true;
bool g_staConnected = false;

uint32_t g_bootMs = 0;
uint32_t g_logStartMs = 0;
uint32_t g_lastInaSampleMs = 0;
uint32_t g_lastTelemetryPushMs = 0;
uint32_t g_lastLogTickMs = 0;
uint32_t g_lastEnergyMs = 0;

uint32_t g_userLogIntervalMs = 1000;
uint32_t g_wsRefreshMs = 200;

bool g_stopImaxEn = false;
bool g_stopIminEn = false;
bool g_stopVmaxEn = false;
bool g_stopVminEn = false;
bool g_stopIntervalEn = false;
float g_stopImax_A = 0.0f;
float g_stopImin_A = 0.0f;
float g_stopVmax_V = 0.0f;
float g_stopVmin_V = 0.0f;
uint32_t g_stopInterval_s = 0;
String g_stopReason = "";

float g_vGain = 1.0f;
float g_iGain = 1.0f;

float g_vOffset_V = 0.0f;
float g_iOffset_A = 0.0f;

float g_busV_raw = 0.0f;
float g_shuntmV_raw = 0.0f;
float g_currentA_raw = 0.0f;
float g_voltageV = 0.0f;
float g_currentA = 0.0f;
float g_power_mW = 0.0f;
double g_energy_mWh = 0.0;

float g_busV_avg = 0.0f;
float g_currentA_avg = 0.0f;

volatile bool g_logWritePulse = false;
volatile uint32_t g_logPulseId = 0;

String g_staSsid = "";
String g_staPass = "";

String g_wifiSsidOptionsHtml = "";
volatile bool g_wifiScanRequested = false;

void initGlobals() {
  g_bootMs = millis();

  g_prefs.begin("labsupply", false);

  g_vGain = g_prefs.getFloat("v_gain", 1.0f);
  g_iGain = g_prefs.getFloat("i_gain", 1.0f);
  g_vOffset_V = g_prefs.getFloat("v_off", 0.0f);
  g_iOffset_A = g_prefs.getFloat("i_off", 0.0f);

  g_staSsid = g_prefs.getString("sta_ssid", "");
  g_staPass = g_prefs.getString("sta_pass", "");

  g_wifiSsidOptionsHtml = "";
  g_wifiScanRequested = true;
}
