// WiFi manager.
//
// Responsibilities:
// - Start and manage AP/STA modes.
// - Provide non-blocking WiFi scan for the /wifi page.
// - Attempt STA connection when credentials exist, with a timeout fallback to AP.
// - Initialize NTP time (Romania local time with DST) when STA becomes connected.
// - Expose basic status via globals used by Web UI.

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <time.h>

#include "Globals.h"
#include "WifiMgr.h"

static const char* AP_SSID = "Lab_Supply";
static const char* AP_PASS = "";
static const char* MDNS_NAME = "powersupply";

static uint32_t s_lastReconnectMs = 0;
static uint32_t s_lastScanStartMs = 0;
static bool s_scanInProgress = false;
static uint32_t s_staConnectStartMs = 0;
static const uint32_t s_staConnectTimeoutMs = 10000;
static bool s_timeInitDone = false;

static void initTimeIfNeeded() {
  if (s_timeInitDone) return;
  configTzTime("EET-2EEST,M3.5.0/3,M10.5.0/4", "pool.ntp.org", "time.nist.gov", "time.google.com");
  s_timeInitDone = true;
}

static String escapeHtml(const String& s) {
  String out;
  out.reserve(s.length());
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '\"') out += "&quot;";
    else if (c == '\'') out += "&#39;";
    else out += c;
  }
  return out;
}

static void updateScanCacheIfReady() {
  int res = WiFi.scanComplete();
  if (res == WIFI_SCAN_RUNNING) return;

  if (res >= 0) {
    String opts;
    for (int i = 0; i < res; i++) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) continue;
      opts += "<option value='" + escapeHtml(ssid) + "'>" + escapeHtml(ssid) + "</option>\n";
    }
    g_wifiSsidOptionsHtml = opts;
    WiFi.scanDelete();
  }
  // res can be negative on error; allow retry
  s_scanInProgress = false;
}

bool startApMode() {
  WiFi.mode(WIFI_AP);
  bool ok = (AP_PASS[0] == '\0') ? WiFi.softAP(AP_SSID) : WiFi.softAP(AP_SSID, AP_PASS);
  g_apMode = true;
  g_staConnected = false;
  s_timeInitDone = false;

  if (ok) {
    Serial.printf("[WiFi] AP started: ssid='%s' ip=%s mac=%s\n", WiFi.softAPSSID().c_str(),
                  WiFi.softAPIP().toString().c_str(), WiFi.softAPmacAddress().c_str());
  } else {
    Serial.println("[WiFi] AP start failed");
  }
  return ok;
}

bool startStaMode(const char* ssid, const char* pass) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  g_apMode = false;
  s_staConnectStartMs = millis();
  return true;
}

void initWiFi() {
  if (g_staSsid.length() > 0) {
    startStaMode(g_staSsid.c_str(), g_staPass.c_str());
  } else {
    startApMode();

    // Enable STA as well to allow scanning without dropping AP
    WiFi.mode(WIFI_AP_STA);
  }

  g_wifiScanRequested = true;

  if (!MDNS.begin(MDNS_NAME)) {
    // ignore
  }
}

void tickWiFi() {
  // Non-blocking scan management for /wifi page
  if (g_wifiScanRequested && !s_scanInProgress) {
    uint32_t now = millis();
    s_lastScanStartMs = now;
    g_wifiScanRequested = false;
    s_scanInProgress = true;

    // Scanning requires STA; keep current mode if AP is active
    if (!g_apMode) {
      WiFi.mode(WIFI_STA);
    } else {
      // Ensure STA interface is enabled for scanning while AP is running
      WiFi.mode(WIFI_AP_STA);
    }
    WiFi.scanNetworks(true, true);
  }
  if (s_scanInProgress) {
    updateScanCacheIfReady();

    // Safety: if scan seems stuck/failed, allow retry
    uint32_t now = millis();
    if (s_scanInProgress && (now - s_lastScanStartMs) > 12000) {
      WiFi.scanDelete();
      s_scanInProgress = false;
    }
  }

  if (!g_apMode) {
    if (WiFi.status() == WL_CONNECTED) {
      g_staConnected = true;
      initTimeIfNeeded();
    } else {
      g_staConnected = false;
      uint32_t now = millis();

      // Avoid reconnect/disconnect operations while scanning; they can cancel the scan.
      // Scan will finish (or timeout) and then normal reconnect logic resumes.
      if (s_scanInProgress) return;

      if (s_staConnectStartMs != 0 && (now - s_staConnectStartMs) > s_staConnectTimeoutMs) {
        s_staConnectStartMs = 0;
        startApMode();
        WiFi.mode(WIFI_AP_STA);
        g_wifiScanRequested = true;
        return;
      }

      if (now - s_lastReconnectMs > 5000) {
        s_lastReconnectMs = now;
        if (g_staSsid.length() > 0) {
          WiFi.disconnect();
          WiFi.begin(g_staSsid.c_str(), g_staPass.c_str());
        }
      }
    }
  }
}
