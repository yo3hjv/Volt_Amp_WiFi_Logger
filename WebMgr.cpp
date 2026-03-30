// Web server manager.
//
// Responsibilities:
// - Configure and start the AsyncWebServer.
// - Serve the main UI from SPIFFS (root).
// - Register routes for WebSocket, WiFi configuration, upload manager, and log APIs.

#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

#include "Globals.h"
#include "WebMgr.h"
#include "WsMgr_Internal.h"
#include "WifiWeb.h"

AsyncWebServer g_server(80);

extern void registerWsRoutes(AsyncWebServer& server);
extern void registerUploadRoutes(AsyncWebServer& server);
extern void registerLogRoutes(AsyncWebServer& server);

void initWebServer() {
  g_server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (SPIFFS.exists("/index.html")) {
      req->send(SPIFFS, "/index.html", "text/html");
      return;
    }
    req->send(200, "text/html",
              "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
              "<title>LabSupplyLog</title></head><body><h2>LabSupplyLog</h2>"
              "<p>UI files not found in SPIFFS. Upload index.html, style.css, app.js from /upload.</p>"
              "<p><a href='/upload'>Go to /upload</a></p>"
              "</body></html>");
  });

  // Serve static files from SPIFFS root
  g_server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  registerWsRoutes(g_server);
  initWebSocketHandler(g_server);
  registerWiFiWebRoutes(g_server);
  registerUploadRoutes(g_server);
  registerLogRoutes(g_server);
}

void startWebServer() {
  g_server.begin();
}
