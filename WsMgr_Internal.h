// Internal WebSocket helpers.
//
// This header exposes the AsyncWebSocket instance to other modules
// (e.g. LogMgr for plot point broadcasts) and provides a helper to attach
// the handler to the AsyncWebServer.

#pragma once

#include <ESPAsyncWebServer.h>

AsyncWebSocket& getWebSocket();
void initWebSocketHandler(AsyncWebServer& server);
