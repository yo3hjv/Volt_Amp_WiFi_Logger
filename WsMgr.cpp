// WebSocket manager.
//
// Responsibilities:
// - Expose a WebSocket endpoint (/ws) for fast UI telemetry updates.
// - Accept control commands from the UI (start/stop logging, calibration, WiFi).
// - Periodically broadcast telemetry (V/A/mW/mWh, elapsed time, date/time, SPIFFS usage).
// - Stop logging automatically if the UI disconnects for more than a short grace period.

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <time.h>

#include "Globals.h"
#include "WsMgr.h"
#include "LogMgr.h"
#include "WifiMgr.h"

static AsyncWebSocket s_ws("/ws");
static int s_wsClients = 0;
static uint32_t s_wsLastClientMs = 0;
static const uint32_t s_wsStopGraceMs = 4000;

static void sendStatus(AsyncWebSocketClient* client) {
  StaticJsonDocument<256> doc;
  doc["type"] = "status";
  doc["logging"] = g_loggingActive;
  doc["ap"] = g_apMode;
  doc["sta"] = g_staConnected;
  doc["pulse_id"] = g_logPulseId;
  String out;
  serializeJson(doc, out);
  if (client) client->text(out);
  else s_ws.textAll(out);
}

static void handleCommand(AsyncWebSocketClient* client, const JsonDocument& doc) {
  const char* type = doc["type"] | "";

  if (strcmp(type, "start_log") == 0) {
    if (doc.containsKey("interval_ms")) g_userLogIntervalMs = doc["interval_ms"].as<uint32_t>();

    // Stop conditions (optional)
    g_stopImaxEn = doc["stop_imax_en"] | false;
    g_stopIminEn = doc["stop_imin_en"] | false;
    g_stopVmaxEn = doc["stop_vmax_en"] | false;
    g_stopVminEn = doc["stop_vmin_en"] | false;
    g_stopIntervalEn = doc["stop_t_en"] | false;

    g_stopImax_A = (doc["stop_imax_mA"] | 0.0f) / 1000.0f;
    g_stopImin_A = (doc["stop_imin_mA"] | 0.0f) / 1000.0f;
    g_stopVmax_V = doc["stop_vmax_V"] | 0.0f;
    g_stopVmin_V = doc["stop_vmin_V"] | 0.0f;
    g_stopInterval_s = doc["stop_t_s"] | 0;

    startLogging();
    sendStatus(client);
    return;
  }

  if (strcmp(type, "stop_log") == 0) {
    stopLogging("user");
    sendStatus(client);
    return;
  }

  if (strcmp(type, "cal") == 0) {
    const float rawV = g_busV_avg;
    const float rawI = g_currentA_avg;

    if (doc.containsKey("v_ext")) {
      float vExt = doc["v_ext"].as<float>();
      // Voltage calibration supports both:
      // - tare/zero (Vcal ~= 0): adjust offset so calibrated voltage becomes 0
      // - gain calibration (Vcal > 0): adjust gain while keeping the current offset
      if (fabsf(vExt) < 0.000001f) {
        // Tare: V = rawV * gain + off => off = -rawV * gain
        g_vOffset_V = -(rawV * g_vGain);
        g_prefs.putFloat("v_off", g_vOffset_V);
      } else if (rawV > 0.05f) {
        // Gain update keeping the current offset: V = rawV * gain + off
        g_vGain = (vExt - g_vOffset_V) / rawV;
        g_prefs.putFloat("v_gain", g_vGain);
      }
    }

    if (doc.containsKey("i_ext")) {
      float iExt = doc["i_ext"].as<float>();
      if (fabsf(iExt) < 0.000001f) {
        // Current zero/tare: adjust offset so calibrated current becomes 0
        g_iOffset_A = -(rawI * g_iGain);
        g_prefs.putFloat("i_off", g_iOffset_A);
      } else if (fabsf(rawI) > 0.01f) {
        g_iGain = (iExt - g_iOffset_A) / rawI;
        g_prefs.putFloat("i_gain", g_iGain);
      }
    }

    sendStatus(client);
    return;
  }

  if (strcmp(type, "set_wifi") == 0) {
    const char* ssid = doc["ssid"] | "";
    const char* pass = doc["pass"] | "";
    g_staSsid = ssid;
    g_staPass = pass;
    g_prefs.putString("sta_ssid", g_staSsid);
    g_prefs.putString("sta_pass", g_staPass);
    startStaMode(ssid, pass);
    sendStatus(client);
    return;
  }

  if (strcmp(type, "switch_ap") == 0) {
    startApMode();
    sendStatus(client);
    return;
  }
}

static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                      void* arg, uint8_t* data, size_t len) {
  (void)server;
  (void)client;

  if (type == WS_EVT_CONNECT) {
    s_wsClients++;
    s_wsLastClientMs = millis();
    sendStatus(client);
    return;
  }

  if (type == WS_EVT_DISCONNECT) {
    if (s_wsClients > 0) s_wsClients--;
    if (s_wsClients == 0) {
      s_wsLastClientMs = millis();
    }
    return;
  }

  if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (!info->final || info->index != 0 || info->len != len) return;
    if (info->opcode != WS_TEXT) return;

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, (const char*)data, len);
    if (err) return;

    handleCommand(client, doc);
  }
}

void registerWsRoutes(AsyncWebServer& server) {
  (void)server;
  s_ws.onEvent(onWsEvent);
}

void initWebSocket() {
  // server will add handler in initWebServer via registerWsRoutes
}

void tickTelemetry() {
  uint32_t now = millis();
  if (now - g_lastTelemetryPushMs < g_wsRefreshMs) return;
  g_lastTelemetryPushMs = now;

  if (g_loggingActive && s_wsClients == 0 && s_wsLastClientMs != 0 && (now - s_wsLastClientMs) > s_wsStopGraceMs) {
    stopLogging("ws");
  }

  StaticJsonDocument<384> doc;
  doc["type"] = "telemetry";
  doc["elapsed_s"] = g_loggingActive ? (float)(now - g_logStartMs) / 1000.0f : 0.0f;
  doc["V"] = g_voltageV;
  doc["A"] = g_currentA;
  doc["mW"] = g_power_mW;
  doc["mWh"] = g_energy_mWh;
  doc["pulse_id"] = g_logPulseId;

  tm t;
  time_t ts = time(nullptr);
  if (ts > 1700000000 && localtime_r(&ts, &t)) {
    static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char dateBuf[16];
    char timeBuf[16];
    const char* mon = (t.tm_mon >= 0 && t.tm_mon < 12) ? months[t.tm_mon] : "---";
    snprintf(dateBuf, sizeof(dateBuf), "%02d.%s.%02d", t.tm_mday, mon, (t.tm_year + 1900) % 100);
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    doc["date"] = dateBuf;
    doc["time"] = timeBuf;
  }

  size_t total = SPIFFS.totalBytes();
  size_t used = SPIFFS.usedBytes();
  if (total > 0) {
    doc["spiffs_used_pct"] = (float)used * 100.0f / (float)total;
  }

  String out;
  serializeJson(doc, out);
  s_ws.textAll(out);
}

// Exposed for server addHandler
AsyncWebSocket& getWebSocket() {
  return s_ws;
}

void initWebSocketHandler(AsyncWebServer& server) {
  server.addHandler(&s_ws);
}
