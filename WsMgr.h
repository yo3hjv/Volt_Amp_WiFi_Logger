// WebSocket manager public API.
//
// The WebSocket manager is responsible for telemetry broadcasting and
// handling control/calibration commands from the Web UI.

#pragma once

void initWebSocket();
void tickTelemetry();

// Internal
void registerWsRoutes(class AsyncWebServer& server);
