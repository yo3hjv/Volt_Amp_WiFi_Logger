// Upload / SPIFFS management public API.
//
// Registers HTTP routes for uploading and managing files in SPIFFS.

#pragma once

void initUploadRoutes();

// Internal
void registerUploadRoutes(class AsyncWebServer& server);
