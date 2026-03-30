// Logging manager public API.
//
// Controls CSV logging to SPIFFS and exposes helper routes used by /logs.

#pragma once

void startLogging();
void stopLogging(const char* reason);
void tickLogging();

// Internal
void registerLogRoutes(class AsyncWebServer& server);
