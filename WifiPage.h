// WiFi page HTML generator.

#pragma once

#include <Arduino.h>

String buildWifiPageHtml(const String& msg, const String& ssidOptionsHtml, const String& savedSsid);
