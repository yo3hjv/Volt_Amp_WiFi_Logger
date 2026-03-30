// LabSupplyLog - main Arduino sketch entry point.
//
// Responsibilities:
// - Initialize global state and persisted settings.
// - Initialize storage (SPIFFS), WiFi, and INA219 sampling.
// - Start the Async Web Server and WebSocket telemetry.
// - Run periodic tick functions in the main loop.

#include <Arduino.h>

#include "Globals.h"
#include "StorageMgr.h"
#include "WifiMgr.h"
#include "InaMgr.h"
#include "LogMgr.h"
#include "WebMgr.h"
#include "WsMgr.h"
#include "UploadMgr.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  initGlobals();

  initStorage();
  initWiFi();
  initIna();

  initWebServer();
  initWebSocket();

  startWebServer();
}

void loop() {
  tickWiFi();
  tickIna();
  tickLogging();
  tickTelemetry();
}
