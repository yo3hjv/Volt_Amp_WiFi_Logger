// Logging manager.
//
// Responsibilities:
// - Create and maintain the active CSV log file in SPIFFS.
// - Generate unique log file names:
//   - Prefer local-time timestamp names when NTP time is valid.
//   - Fallback to incremental /log_000x.csv.
// - Append samples at the user-selected interval.
// - Enforce stop conditions (I/V limits and maximum time).
// - Prune old log files to keep a bounded history.
// - Provide basic web API routes for listing and deleting logs.

#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <time.h>

#include <algorithm>
#include <vector>

#include "Globals.h"
#include "LogMgr.h"
#include "WsMgr_Internal.h"

static File s_logFile;
static String s_logFilename;

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isTimestampLogName(const String& name) {
  // /DDMMYY_HHMMSS.csv or /DDMMYY_HHMMSS_1.csv
  String n = name;
  if (!n.startsWith("/")) n = "/" + n;
  if (!n.endsWith(".csv")) return false;
  if (n.length() < 1 + 6 + 1 + 6 + 4) return false;

  for (int i = 1; i <= 6; i++) {
    if (!isDigit(n[i])) return false;
  }
  if (n[7] != '_') return false;
  for (int i = 8; i <= 13; i++) {
    if (!isDigit(n[i])) return false;
  }

  int dot = n.lastIndexOf('.');
  if (dot < 0) return false;
  if (dot == 14) return true;
  if (n[14] != '_') return false;
  for (int i = 15; i < dot; i++) {
    if (!isDigit(n[i])) return false;
  }
  return true;
}

static bool isLogName(const String& name) {
  if (name.startsWith("/log_") && name.endsWith(".csv")) return true;
  return isTimestampLogName(name);
}

static uint32_t findNextLogIndex() {
  uint32_t maxIdx = 0;

  File root = SPIFFS.open("/");
  if (!root) return 1;

  File f = root.openNextFile();
  while (f) {
    File cur = f;
    f = root.openNextFile();

    String n = String(cur.name());
    cur.close();
    if (isLogName(n)) {
      // /log_0001.csv
      int us = n.indexOf('_');
      int dot = n.lastIndexOf('.');
      if (us >= 0 && dot > us) {
        String num = n.substring(us + 1, dot);
        uint32_t idx = (uint32_t)num.toInt();
        if (idx > maxIdx) maxIdx = idx;
      }
    }
  }

  return maxIdx + 1;
}

static String makeLogFilename(uint32_t idx) {
  char buf[20];
  snprintf(buf, sizeof(buf), "/log_%04lu.csv", (unsigned long)idx);
  return String(buf);
}

static bool getLocalTimestamp(tm& outTm) {
  time_t now = time(nullptr);
  if (now < 1700000000) return false;
  return localtime_r(&now, &outTm) != nullptr;
}

static String makeTimestampFilename(const tm& t) {
  char buf[32];
  snprintf(buf, sizeof(buf), "/%02d%02d%02d_%02d%02d%02d.csv", t.tm_mday, t.tm_mon + 1, (t.tm_year + 1900) % 100,
           t.tm_hour, t.tm_min, t.tm_sec);
  return String(buf);
}

static String ensureUniqueFilename(const String& basePath) {
  if (!SPIFFS.exists(basePath)) return basePath;

  int dot = basePath.lastIndexOf('.');
  String prefix = basePath;
  String ext = "";
  if (dot > 0) {
    prefix = basePath.substring(0, dot);
    ext = basePath.substring(dot);
  }
  for (int i = 1; i <= 99; i++) {
    String candidate = prefix + "_" + String(i) + ext;
    if (!SPIFFS.exists(candidate)) return candidate;
  }
  return basePath;
}

static void pruneOldLogsKeepMax(size_t keepMax) {
  if (keepMax == 0) return;
  std::vector<String> logs;

  File root = SPIFFS.open("/");
  if (!root) return;

  File f = root.openNextFile();
  while (f) {
    File cur = f;
    f = root.openNextFile();
    String n = String(cur.name());
    cur.close();
    if (isLogName(n)) logs.push_back(n);
  }

  if (logs.size() <= keepMax) return;

  std::sort(logs.begin(), logs.end());
  const size_t toDelete = logs.size() - keepMax;
  for (size_t i = 0; i < toDelete; i++) {
    if (logs[i] != s_logFilename) {
      SPIFFS.remove(logs[i]);
    }
  }
}

void startLogging() {
  if (!g_spiffsOk) return;

  if (g_loggingActive) return;

  g_stopReason = "";
  g_energy_mWh = 0.0;
  g_lastEnergyMs = millis();

  tm t;
  if (getLocalTimestamp(t)) {
    s_logFilename = ensureUniqueFilename(makeTimestampFilename(t));
  } else {
    uint32_t idx = findNextLogIndex();
    s_logFilename = ensureUniqueFilename(makeLogFilename(idx));
  }

  s_logFile = SPIFFS.open(s_logFilename, "w");
  if (!s_logFile) {
    g_loggingActive = false;
    return;
  }

  s_logFile.println("elapsed_s,V,A,mW,mWh");
  s_logFile.flush();

  g_logStartMs = millis();
  g_lastLogTickMs = g_logStartMs;
  g_loggingActive = true;

  pruneOldLogsKeepMax(50);
}

void stopLogging(const char* reason) {
  if (!g_loggingActive) return;

  g_loggingActive = false;
  g_stopReason = reason ? String(reason) : String("stop");

  if (s_logFile) {
    s_logFile.close();
  }
}

static bool checkStopConditions(uint32_t now) {
  if (g_stopImaxEn && g_currentA > g_stopImax_A) {
    stopLogging("Imax");
    return true;
  }
  if (g_stopIminEn && g_currentA < g_stopImin_A) {
    stopLogging("Imin");
    return true;
  }
  if (g_stopVmaxEn && g_voltageV > g_stopVmax_V) {
    stopLogging("Vmax");
    return true;
  }
  if (g_stopVminEn && g_voltageV < g_stopVmin_V) {
    stopLogging("Vmin");
    return true;
  }
  if (g_stopIntervalEn) {
    uint32_t elapsed_s = (now - g_logStartMs) / 1000;
    if (elapsed_s >= g_stopInterval_s) {
      stopLogging("Tmax");
      return true;
    }
  }
  return false;
}

static void emitPlotPoint(uint32_t now) {
  StaticJsonDocument<256> doc;
  doc["type"] = "plot_point";
  doc["t"] = (float)(now - g_logStartMs) / 1000.0f;
  doc["V"] = g_voltageV;
  doc["A"] = g_currentA;
  doc["pulse_id"] = g_logPulseId;

  String out;
  serializeJson(doc, out);
  getWebSocket().textAll(out);
}

void tickLogging() {
  if (!g_loggingActive) return;
  if (!s_logFile) {
    stopLogging("file");
    return;
  }

  uint32_t now = millis();
  if (checkStopConditions(now)) {
    return;
  }

  if (now - g_lastLogTickMs < g_userLogIntervalMs) return;
  g_lastLogTickMs = now;

  float elapsed_s = (float)(now - g_logStartMs) / 1000.0f;

  char line[128];
  // Use fixed decimals; Arduino snprintf float support depends on build flags, but ESP32 supports it.
  snprintf(line, sizeof(line), "%.3f,%.3f,%.4f,%.1f,%.6f", elapsed_s, g_voltageV, g_currentA, g_power_mW, (float)g_energy_mWh);
  s_logFile.println(line);
  s_logFile.flush();

  g_logWritePulse = true;
  g_logPulseId++;

  emitPlotPoint(now);
}

void registerLogRoutes(AsyncWebServer& server) {
  server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest* req) {
    String json = "[";
    bool first = true;

    File root = SPIFFS.open("/");
    File f = root.openNextFile();
    while (f) {
      File cur = f;
      f = root.openNextFile();
      String n = String(cur.name());
      cur.close();
      if (isLogName(n)) {
        if (!first) json += ",";
        first = false;
        json += "\"" + n.substring(1) + "\""; // without leading '/'
      }
    }
    json += "]";
    req->send(200, "application/json", json);
  });

  server.on("/api/logs/delete_all", HTTP_POST, [](AsyncWebServerRequest* req) {
    File root = SPIFFS.open("/");
    File f = root.openNextFile();
    while (f) {
      File cur = f;
      f = root.openNextFile();
      String n = String(cur.name());
      cur.close();
      if (isLogName(n)) {
        SPIFFS.remove(n);
      }
    }
    req->send(200, "text/plain", "OK");
  });

  // Serve log downloads via /log?name=log_0001.csv
  server.on("/log", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (!req->hasParam("name")) {
      req->send(400, "text/plain", "missing name");
      return;
    }
    String name = req->getParam("name")->value();
    if (!name.startsWith("log_") || !name.endsWith(".csv")) {
      req->send(400, "text/plain", "bad name");
      return;
    }
    String path = "/" + name;
    if (!SPIFFS.exists(path)) {
      req->send(404, "text/plain", "not found");
      return;
    }
    req->send(SPIFFS, path, "text/csv", true);
  });
}
